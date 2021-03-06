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

#include "routines/CRoutineMaker.h"

#include "Core1.h"
#include <string.h>
#include "pico/multicore.h"

/* 
 * For code that runs on core1
 */

static Core1 *core1 = NULL;

void core1_entry() 
{
    printf("Core1::core1_entry()\n");
    core1->init();

    while(1)
        core1->loop();
}

Core1* core1_start(std::vector<CRoutineMaker*> *routines, CSavedSettings *saved_settings) 
{   
    printf("core1_start\n");
    if (core1 == NULL)
    {
        core1 = new Core1(routines, saved_settings);
    }
    
    multicore_reset_core1(); // Try and fix issue with random hangs when debugging (on start)
    sleep_ms(100);
    multicore_launch_core1(core1_entry);
    sleep_ms(10);
    return core1;
}

Core1::Core1(std::vector<CRoutineMaker*> *routines, CSavedSettings *saved_settings)
{
    printf("Core1::Core1()\n");
    _saved_settings = saved_settings;
    _active_routine = NULL;
    _channel_config = NULL;

    _routines = routines;
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

    for (uint8_t channel_number=0; channel_number < MAX_CHANNELS; channel_number++)
        if (_active_channels[channel_number] != NULL)
        {
            _active_channels[channel_number]->loop(time_us_64());
            _active_channels[channel_number]->update_power();
        }

    power_level_control->loop();

    if (_channel_config != NULL)
        _channel_config->loop();

#ifndef SINGLE_CORE
    update_power_levels();
    process_messages();
#endif
}

void Core1::update_power_levels()
{
    for (uint8_t channel_number=0; channel_number < MAX_CHANNELS; channel_number++)
    {
        // Send current power level being output, if changed
        uint16_t power_level = power_level_control->get_output_power_level(channel_number);
        if (power_level != _output_power[channel_number])
        {
            message msg = {0};
            msg.msg8[0] = MESSAGE_SET_POWER;
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

void Core1::process_messages()
{
    while (multicore_fifo_rvalid())
    {
        message msg;
        msg.msg32 = multicore_fifo_pop_blocking();
        process_message(msg);
    }
}

void Core1::process_message(message msg)
{
    printf("Core1::process_message(): got msg type %d (%d, %d, %d)\n", msg.msg8[0], msg.msg8[1], msg.msg8[2], msg.msg8[3]);
    switch(msg.msg8[0])
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
            bool pushed        =  msg.msg8[2];
            soft_button_pushed(button, pushed);
            break;
        }

        case MESSAGE_REINIT_CHANNELS:
            stop_routine();
            init();
            break;
    }
}

void Core1::activate_routine(uint8_t routine_id)
{
    CRoutineMaker* routine_maker = (*_routines)[routine_id];    
    
    if (!routine_maker)
    {
        printf("CMenuRoutineSelection::activate_routine NULL routine - exit\n");
        return;
    }
    
    stop_routine();

    _active_routine = routine_maker();

    routine_conf conf;
    _active_routine->get_config(&conf);

    // Loop through all the channels the routine has requsted
    uint8_t channel=0;
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
                        _fullChannelAsSimpleChannels[channel] = new CFullChannelAsSimpleChannel(_saved_settings, (CFullOutputChannel*)_active_channels[channel], channel, power_level_control);
                        _active_channels[channel] = _fullChannelAsSimpleChannels[channel];
                    }

                    if (_active_channels[channel]->get_channel_type() == COutputChannel::channel_type::SIMPLE)
                    {
                        _active_routine->set_simple_output_channel(channel, (CSimpleOutputChannel*)_active_channels[channel]);
                    }

                    else
                    {
                        printf("ERROR: Unknown channel type for channel (%d)\n", channel);
                    }
                    break;

                case output_type::FULL:
                    if (_active_channels[channel]->get_channel_type() == COutputChannel::channel_type::FULL)
                    {
                        _active_routine->set_full_output_channel(channel, (CFullOutputChannel*)_active_channels[channel]);
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
}

void Core1::stop_routine()
{
    // Stop & delete currently running routine, if any
    if (_active_routine != NULL)
    {
        (_active_routine)->stop();
        delete _active_routine;
        _active_routine = NULL;
    }

    set_output_chanels_to_off();

    // Get rid of any FullChannelAsSimpleChannel wrappers that may have been used
    delete_fullChannelAsSimpleChannels_and_restore_channels();
    set_output_chanels_to_off();
}

void Core1::set_output_chanels_to_off()
{
    // set power levels to min/off
    for (uint8_t channel_number=0; channel_number < MAX_CHANNELS; channel_number++)
    {
        if (_active_channels[channel_number] != NULL)
        {
            _active_channels[channel_number]->channel_set_power(0);
        }
    }

    power_level_control->zero_power_level();
    update_power_levels();
}

void Core1::delete_fullChannelAsSimpleChannels_and_restore_channels()
{
    for (int x=0; x < MAX_CHANNELS; x++)
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

void Core1::collar_transmit (uint16_t id, CCollarComms::collar_channel channel, CCollarComms::collar_mode mode, uint8_t power)
{
    CCollarComms *collar_comms = _channel_config->get_collar_comms();

    CCollarComms::collar_message msg;
    msg.id = id;
    msg.mode = mode;
    msg.channel = channel;
    msg.power = power;

    collar_comms->transmit(msg);
}
