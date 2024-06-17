/*
 * ZC95
 * Copyright (C) 2021  CrashOverride85
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "Core1.h"
#include "../globals.h"
#include "../gDebugCounters.h"
#include <string.h>
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include <malloc.h>

/*
 * For code that runs on core1
 */

#define STACK_SIZE 6144
static Core1 *core1 = NULL;
static uint32_t *_stack = NULL;

void core1_entry()
{
    printf("Core1::core1_entry()\n");
    core1->init();

    while (1)
        core1->loop();
}

Core1 *core1_start(std::vector<CRoutines::Routine>& routines, CSavedSettings *saved_settings)
{
    printf("core1_start\n");
    if (core1 == NULL)
    {
        core1 = new Core1(routines, saved_settings);
    }
    if (_stack == NULL)
    {
        _stack = (uint32_t *)malloc(STACK_SIZE);
    }

    multicore_reset_core1(); // Try and fix issue with random hangs when debugging (on start)
    sleep_ms(100);

 // multicore_launch_core1(core1_entry);
    multicore_launch_core1_with_stack(core1_entry, _stack, STACK_SIZE);

    sleep_ms(10);
    return core1;
}

Core1::Core1(std::vector<CRoutines::Routine>& routines, CSavedSettings *saved_settings)  : _routines(routines)
{
    printf("Core1::Core1()\n");
    _saved_settings = saved_settings;
    _active_routine = NULL;
    _channel_config = NULL;

    power_level_control = new CPowerLevelControl(saved_settings);

    memset(_active_channels, 0, sizeof(_active_channels));
    memset(_fullChannelAsSimpleChannels, 0, sizeof(_fullChannelAsSimpleChannels));
}

void Core1::init()
{
    // Note: If channel config is changed, this will be called again
    printf("Core1::init()\n");

    for (int x = 0; x < MAX_CHANNELS; x++)
    {
        if (_fullChannelAsSimpleChannels[x] != NULL)
        {
            delete _fullChannelAsSimpleChannels[x];
            _fullChannelAsSimpleChannels[x] = NULL;
        }

        if (_active_channels[x] != NULL)
        {
            delete _active_channels[x];
            _active_channels[x] = NULL;
        }
    }

    if (_channel_config != NULL)
    {
        delete _channel_config;
        _channel_config = NULL;
    }

    _channel_config = new CChannelConfig(_saved_settings, power_level_control);
    _channel_config->configure_channels_from_saved_config(_active_channels);

    memset(_fullChannelAsSimpleChannels, 0, sizeof(_fullChannelAsSimpleChannels));
    for (int x = 0; x < MAX_CHANNELS; x++)
    {
        _real_output_channel[x] = _active_channels[x];
    }
}

Core1::~Core1()
{
    printf("Core1::~Core1()\n");
    delete _channel_config;
    delete_fullChannelAsSimpleChannels_and_restore_channels();

    if (power_level_control != NULL)
    {
        delete power_level_control;
        power_level_control = NULL;
    }
}

void Core1::loop()
{
    if (_active_routine != NULL)
    {
        _active_routine->loop(time_us_64());
    }

    for (uint8_t channel_number = 0; channel_number < MAX_CHANNELS; channel_number++)
        if (_active_channels[channel_number] != NULL)
        {
            _active_channels[channel_number]->loop(time_us_64());
            _active_channels[channel_number]->update_power();
        }

    power_level_control->loop();

    if (_channel_config != NULL)
        _channel_config->loop();

    update_power_levels();
    process_messages();
    check_validity_of_lua_script();


    if (gFatalError)
    {
        for (uint8_t channel_number = 0; channel_number < MAX_CHANNELS; channel_number++)
        {
            if (_active_channels[channel_number] != NULL)
            {
                _active_channels[channel_number]->channel_set_power(0);
                _active_channels[channel_number]->loop(time_us_64());
                _active_channels[channel_number]->update_power();
            }
        }

        _channel_config->shutdown_zc624();
        printf("Core1: HALT.\n");
        while (1);
    }
}

void Core1::update_power_levels()
{
    for (uint8_t channel_number = 0; channel_number < MAX_CHANNELS; channel_number++)
    {
        // Send current power level being output, if changed
        uint16_t power_level = power_level_control->get_display_power_level(channel_number);
        if (power_level != _output_power[channel_number])
        {
            message msg = {0};
            msg.msg8[0] = MESSAGE_SET_DISPLAY_POWER;
            msg.msg8[1] = channel_number;
            msg.msg8[2] = power_level & 0xFF;
            msg.msg8[3] = (power_level >> 8) & 0xFF;

            if (multicore_fifo_wready())
            {
                // printf("Core1::update_power_levels(): send power level\n");
                multicore_fifo_push_blocking(msg.msg32);
                _output_power[channel_number] = power_level;
            }
        }

        // Send the current maximum power (this will be increasing automatically during ramp up)
        uint16_t power_level_max = power_level_control->get_max_power_level(channel_number);
        if (power_level_max != _output_power_max[channel_number])
        {
            message msg = {0};
            msg.msg8[0] = MESSAGE_SET_MAXIMUM_POWER;
            msg.msg8[1] = channel_number;
            msg.msg8[2] = power_level_max & 0xFF;
            msg.msg8[3] = (power_level_max >> 8) & 0xFF;

            if (multicore_fifo_wready())
            {
                multicore_fifo_push_blocking(msg.msg32);
                _output_power_max[channel_number] = power_level_max;
            }
        }
    }
}

void Core1::check_validity_of_lua_script()
{
    lua_script_state_t state = lua_script_state_t::NOT_APPLICABLE;

    if (_active_routine != NULL)
    {
        state = _active_routine->lua_script_state();
    }

    if (_script_script_state != state)
    {
        message msg = {0};
        msg.msg8[0] = MESSAGE_LUA_SCRIPT_STATE;
        msg.msg8[1] = (uint8_t)state;

        if (multicore_fifo_wready())
        {
            multicore_fifo_push_blocking(msg.msg32);
            _script_script_state = state;
        }
    }
}

void Core1::process_messages()
{
    // main FIFO queue - instructions to start routines, change settings, etc.
    while (multicore_fifo_rvalid())
    {
        message msg;
        msg.msg32 = multicore_fifo_pop_blocking();
        process_message(msg);
    }

    // Pulses from audio processing or remote access that don't fit in the usual FIFO queue
    process_audio_pulse_queue();
}

void Core1::process_message(message msg)
{
    // printf("Core1::process_message(): got msg type %d (%d, %d, %d)\n", msg.msg8[0], msg.msg8[1], msg.msg8[2], msg.msg8[3]);
    switch (msg.msg8[0])
    {
    case MESSAGE_ROUTINE_LOAD:
        activate_routine(msg.msg8[1]);
        break;

    case MESSAGE_ROUTINE_STOP:
        stop_routine();
        break;

    case MESSAGE_ROUTINE_MIN_MAX_CHANGE:
    {
        uint8_t menu_id = msg.msg8[1];
        uint16_t new_value = msg.msg8[2];
        new_value |= msg.msg8[3] << 8;
        menu_min_max_change(menu_id, new_value);
        break;
    }

    case MESSAGE_ROUTINE_MULTI_CHOICE_CHANGE:
    {
        uint8_t menu_id = msg.msg8[1];
        uint16_t choice_id = msg.msg8[2];
        choice_id |= msg.msg8[3] << 8;
        menu_multi_choice_change(menu_id, choice_id);
        break;
    }

    case MESSAGE_ROUTINE_MENU_SELECTED:
    {
        uint8_t menu_id = msg.msg8[1];
        menu_selected(menu_id);
        break;
    }

    case MESSAGE_ROUTINE_TRIGGER:
    {
        uint8_t socket = msg.msg8[1];
        uint16_t part = msg.msg8[2];
        bool active = msg.msg8[3];
        trigger((trigger_socket)socket, (trigger_part)part, active);
        break;
    }

    case MESSAGE_SET_FRONT_PANNEL_POWER:
    {
        uint8_t channel = msg.msg8[1];
        uint16_t power = msg.msg8[2];
        power |= msg.msg8[3] << 8;
        power_level_control->set_front_panel_power(channel, power);
        update_channel_power(channel);
        break;
    }

    case MESSAGE_TRIGGER_COLLAR:
    {
        if (mutex_enter_block_until(&g_collar_message_mutex, 0))
        {
            collar_transmit(g_collar_message.id, g_collar_message.channel, g_collar_message.mode, g_collar_message.power);
            mutex_exit(&g_collar_message_mutex);
        }
        else
        {
            printf("Core1::process_message(): Failed to get g_collar_message_mutex, message ignored\n");
        }
        break;
    }

    case MESSAGE_ROUTINE_SOFT_BUTTON_PUSHED:
    {
        soft_button button = (soft_button)msg.msg8[1];
        bool pushed = msg.msg8[2];
        soft_button_pushed(button, pushed);
        break;
    }

    case MESSAGE_REINIT_CHANNELS:
        stop_routine();
        init();
        break;

    case MESSAGE_AUDIO_THRES_REACHED:
        if (_active_routine != NULL)
        {
            uint16_t fundamental_freq = msg.msg8[1];
            fundamental_freq |= msg.msg8[2] << 8;
            uint8_t cross_count = msg.msg8[3];
            _active_routine->audio_threshold_reached(fundamental_freq, cross_count);
        }
        break;

    case MESSAGE_AUDIO_INTENSITY:
        if (_active_routine != NULL)
        {
            uint8_t left_chan = msg.msg8[1];
            uint8_t right_chan = msg.msg8[2];
            uint8_t virt_chan = msg.msg8[3];
            _active_routine->audio_intensity(left_chan, right_chan, virt_chan);
        }
        break;

    case MESSAGE_CORE1_SUSPEND:
        printf("Core1: suspending\n");
        core1_suspend();
        printf("Core1: resumed\n");
        break;   
        
    case MESSAGE_SET_REMOTE_ACCESS_POWER:
        {
            uint8_t channel = msg.msg8[1];
            uint16_t power = msg.msg8[2];
            power |= msg.msg8[3] << 8;
            power_level_control->set_remote_power(channel, power);
            update_channel_power(channel);
            break;
        }

    case MESSAGE_SET_REMOTE_ACCESS_MODE:
        {
            uint8_t enable = (msg.msg8[1] != 0);
            if (enable)
                power_level_control->remote_mode_enable();
            else
                power_level_control->remote_mode_disable();

            for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
                update_channel_power(channel);
            break;
        }

    case MESSAGE_BLUETOOTH_REMOTE_KEYPRESS:
        {
            CBluetoothRemote::keypress_t button = (CBluetoothRemote::keypress_t)msg.msg8[1];
            bluetooth_remote_keypress(button);
            break;
        }
    }
}

void __not_in_flash_func(Core1::core1_suspend)(void)
{
    uint32_t save = save_and_disable_interrupts();

    // Core0 will have acquired this sem before suspending this core. Let it know it can continue by releasing it.
    sem_release(&g_core1_suspend_sem);

    mutex_enter_blocking(&g_core1_suspend_mutex);

    restore_interrupts(save);

    mutex_exit(&g_core1_suspend_mutex);
}

void Core1::process_audio_pulse_queue()
{    
    pulse_message_t pulse_message;
    for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
    {
        if (queue_try_peek (&gPulseQueue[channel], &pulse_message))
        {
            // Check message isn't too old
            if (pulse_message.abs_time_us <  time_us_64()-1000)
            {
                // This shouldn't really happen. Any pulses with a time in the past would already have been discarded on reception
                // Assuming a pulse was received that was due now, it should still reach point within 1000us
                debug_counters_increment(dbg_counter_t::DBG_COUNTER_MSG_OLD);
                queue_try_remove(&gPulseQueue[channel], &pulse_message); // discard message
                continue;
            }

            // Check message isn't too far in the future
            if (pulse_message.abs_time_us > time_us_64() + (1000*1000))
            {
                debug_counters_increment(dbg_counter_t::DBG_COUNTER_MSG_FUTURE);
                queue_try_remove(&gPulseQueue[channel], &pulse_message); // message is for more than a second in the future. That's probably a bug somewhere; discard
                continue;
            }

            if (time_us_64() >= pulse_message.abs_time_us)
            {
                queue_try_remove(&gPulseQueue[channel], &pulse_message);
                if (_active_routine)
                {
                    _active_routine->pulse_message(channel, pulse_message.power_level, pulse_message.pos_pulse_us, pulse_message.neg_pulse_us);
                }
            }
        }
    }
}


void Core1::activate_routine(uint8_t routine_id)
{
    printf("Core1::activate_routine(%d)\n", routine_id);
    CRoutines::Routine routine = _routines[routine_id];

    if (!routine.routine_maker)
    {
        printf("CMenuRoutineSelection::activate_routine NULL routine - exit\n");
        return;
    }

    stop_routine();

    _active_routine= routine.routine_maker(routine.param);

    routine_conf conf;
    _active_routine->get_config(&conf);

    // Loop through all the channels the routine has requested
    uint8_t channel = 0;
    for (std::vector<output_type>::iterator it = conf.outputs.begin(); it != conf.outputs.end(); it++)
    {
        if (_active_channels[channel] == NULL)
        {
            printf("Core1::activate_routine(): ERROR - _active_channels[%d] == NULL\n", channel);
        }
        else
        {
            switch (*it)
            {
            case output_type::SIMPLE:
                if (_active_channels[channel]->get_channel_type() == COutputChannel::channel_type::FULL)
                {
                    // Routine wants a simple channel, but that channel is a full one. So use a wrapper to convert it into a simple channel
                    _fullChannelAsSimpleChannels[channel] = new CFullChannelAsSimpleChannel(_saved_settings, (CFullOutputChannel *)_active_channels[channel], channel, power_level_control);
                    _active_channels[channel] = _fullChannelAsSimpleChannels[channel];
                }

                if (_active_channels[channel]->get_channel_type() == COutputChannel::channel_type::SIMPLE)
                {
                    _active_routine->set_simple_output_channel(channel, (CSimpleOutputChannel *)_active_channels[channel]);
                }

                else
                {
                    printf("ERROR: Unknown channel type for channel (%d)\n", channel);
                }
                break;

            case output_type::FULL:
                if (_active_channels[channel]->get_channel_type() == COutputChannel::channel_type::FULL)
                {
                    _active_routine->set_full_output_channel(channel, (CFullOutputChannel *)_active_channels[channel]);
                }
                else
                {
                    printf("CMenuRoutineSelection::activate_routine(): ERROR - routine requested FULL output channel, but chan %d is not type FULL\n", channel);
                }
                break;

            default:
                printf("CMenuRoutineSelection::activate_routine(): ERROR - unexpected output_type\n");
                break;
            }
        }

        channel++;
    }

    power_level_control->ramp_start();
    _active_routine->start();
    set_audio_mode(conf.audio_processing_mode);
    printf("Core1::activate_routine: completed\n");
}

void Core1::stop_routine()
{
    // Stop & delete currently running routine, if any
    if (_active_routine != NULL)
    {
        printf("Stop routine\n");
        (_active_routine)->stop();
        delete _active_routine;
        _active_routine = NULL;
    }

    set_output_chanels_to_off(true);

    // Get rid of any FullChannelAsSimpleChannel wrappers that may have been used
    delete_fullChannelAsSimpleChannels_and_restore_channels();
    set_output_chanels_to_off(false);
    set_audio_mode(audio_mode_t::OFF);
}

void Core1::set_output_chanels_to_off(bool enable_channel_isolation)
{
    // set power levels to min/off
    for (uint8_t channel_number = 0; channel_number < MAX_CHANNELS; channel_number++)
    {
        if (_active_channels[channel_number] != NULL)
        {
            _active_channels[channel_number]->channel_set_power(0);

            // Make sure ChannelIsolation is on ready for the next routine. This should happen anyway, but just to make sure.
            if (enable_channel_isolation && _active_channels[channel_number]->get_channel_type() == COutputChannel::channel_type::FULL)
            {
                ((CFullOutputChannel *)_active_channels[channel_number])->set_channel_isolation(true);
            }
        }
    }

    power_level_control->zero_power_level();
    update_power_levels();
}

void Core1::delete_fullChannelAsSimpleChannels_and_restore_channels()
{
    for (int x = 0; x < MAX_CHANNELS; x++)
    {
        if (_fullChannelAsSimpleChannels[x] != NULL)
        {
            delete _fullChannelAsSimpleChannels[x];
            _fullChannelAsSimpleChannels[x] = NULL;
        }

        _active_channels[x] = _real_output_channel[x];
    }
}

void Core1::update_channel_power(uint8_t channel)
{
    if (channel > MAX_CHANNELS)
        return;

    if (_active_channels[channel] != NULL)
        _active_channels[channel]->update_power();
}

void Core1::menu_min_max_change(uint8_t menu_id, int16_t new_value)
{
    if (_active_routine != NULL)
    {
        _active_routine->menu_min_max_change(menu_id, new_value);
    }
}

void Core1::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    if (_active_routine != NULL)
    {
        _active_routine->menu_multi_choice_change(menu_id, choice_id);
    }
}

void Core1::menu_selected(uint8_t menu_id)
{
    if (_active_routine != NULL)
    {
        _active_routine->menu_selected(menu_id);
    }
}

void Core1::trigger(trigger_socket socket, trigger_part part, bool active)
{
    if (_active_routine != NULL)
    {
        _active_routine->trigger(socket, part, active);
    }
}

void Core1::soft_button_pushed(soft_button button, bool pushed)
{
    if (_active_routine != NULL)
    {
        _active_routine->soft_button_pushed(button, pushed);
    }
}

void Core1::collar_transmit(uint16_t id, CCollarComms::collar_channel channel, CCollarComms::collar_mode mode, uint8_t power)
{
    CCollarComms *collar_comms = _channel_config->get_collar_comms();

    CCollarComms::collar_message msg;
    msg.id = id;
    msg.mode = mode;
    msg.channel = channel;
    msg.power = power;

    collar_comms->transmit(msg);
}

void Core1::bluetooth_remote_keypress(CBluetoothRemote::keypress_t key)
{
    if (_active_routine != NULL)
    {
        _active_routine->bluetooth_remote_keypress(key);
    }
}

void Core1::set_audio_mode(audio_mode_t mode)
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_SET_AUDIO_MODE;
    msg.msg8[1] = (uint8_t)mode;

    if (multicore_fifo_wready())
    {
        multicore_fifo_push_blocking(msg.msg32);
    }
}
