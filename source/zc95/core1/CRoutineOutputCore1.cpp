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

#include "pico/multicore.h"
#include "pico/mutex.h"

#include "../EExtInputPort.h"
#include "CRoutineOutputCore1.h"
#include "Core1Messages.h"


CRoutineOutputCore1::CRoutineOutputCore1(CDisplay *display, CLedControl *led_control, CExtInputPortExp **ext_port_exp)
{
    _display = display;
    _led_control = led_control;
    _ext_port_exp = ext_port_exp;
}

void CRoutineOutputCore1::set_front_panel_power(uint8_t channel, uint16_t power)
{
    if (channel > MAX_CHANNELS)
        return;
    
    if (_front_pannel_power[channel] != power)
    {
        message msg = {0};
        msg.msg8[0] = MESSAGE_SET_FRONT_PANNEL_POWER;
        msg.msg8[1] = channel;
        msg.msg8[2] = power & 0xFF;
        msg.msg8[3] = (power >> 8) & 0xFF;

        _front_pannel_power[channel] = power;
        multicore_fifo_push_blocking(msg.msg32);
        update_display(channel);
    }
}

uint16_t CRoutineOutputCore1::get_output_power(uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return 0;

    return _output_power[channel]; 
}

uint16_t CRoutineOutputCore1::get_front_pannel_power(uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return 0;

    return _front_pannel_power[channel];
}

uint16_t CRoutineOutputCore1::get_max_output_power(uint8_t channel){
    if (channel >= MAX_CHANNELS)
        return 0;

    return _max_output_power[channel];
}

void CRoutineOutputCore1::activate_routine(uint8_t routine_id)
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_ROUTINE_LOAD;
    msg.msg8[1] = routine_id;

    multicore_fifo_push_blocking(msg.msg32);
}

void CRoutineOutputCore1::stop_routine()
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_ROUTINE_STOP;

    multicore_fifo_push_blocking(msg.msg32);
}

void CRoutineOutputCore1::update_display(uint8_t channel)
{
    _display->set_power_level(channel, get_front_pannel_power(channel), get_output_power(channel), get_max_output_power(channel));
}

void CRoutineOutputCore1::menu_min_max_change(uint8_t menu_id, int16_t new_value)
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_ROUTINE_MIN_MAX_CHANGE;
    msg.msg8[1] = menu_id;
    msg.msg8[2] = new_value & 0xFF;
    msg.msg8[3] = (new_value >> 8) & 0xFF;

    multicore_fifo_push_blocking(msg.msg32);
}

void CRoutineOutputCore1::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_ROUTINE_MULTI_CHOICE_CHANGE;
    msg.msg8[1] = menu_id;
    msg.msg8[2] = choice_id & 0xFF;
    msg.msg8[3] = (choice_id >> 8) & 0xFF;

    multicore_fifo_push_blocking(msg.msg32);
}

void CRoutineOutputCore1::trigger(trigger_socket socket, trigger_part part, bool active)
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_ROUTINE_TRIGGER;
    msg.msg8[1] = (uint8_t)socket;
    msg.msg8[2] = (uint8_t)part;
    msg.msg8[3] = active;

    multicore_fifo_push_blocking(msg.msg32);
}

void CRoutineOutputCore1::soft_button_pressed(soft_button button, bool pressed)
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_ROUTINE_SOFT_BUTTON_PUSHED;
    msg.msg8[1] = (uint8_t)button;
    msg.msg8[2] = pressed; // 1=pressed, 0=released

    multicore_fifo_push_blocking(msg.msg32);
}

void CRoutineOutputCore1::loop()
{
    // Process inbound messages from Core1
    while (multicore_fifo_rvalid())
    {
        message msg;
        msg.msg32 = multicore_fifo_pop_blocking();
        // printf("CRoutineOutputCore1::loop(): get message (%d\t%d\t%d\t%d\n", msg.msg8[0], msg.msg8[1], msg.msg8[2], msg.msg8[3]);
        process_message(msg);
    }
}

// Process message received from core1
void CRoutineOutputCore1::process_message(message msg)
{
    switch(msg.msg8[0])
    {
        case MESSAGE_SET_POWER:
        {
            uint8_t channel = msg.msg8[1];
            uint16_t power = msg.msg8[2];
            power |= msg.msg8[3] << 8;
            
            if (channel >= MAX_CHANNELS)
                return;
            
            _output_power[channel] = power;
            update_display(channel);
            break;
        }

        case MESSAGE_SET_MAXIMUM_POWER:
        {
            uint8_t channel = msg.msg8[1];
            uint16_t power = msg.msg8[2];
            power |= msg.msg8[3] << 8;
            
            if (channel >= MAX_CHANNELS)
                return;
            
            _max_output_power[channel] = power;
            update_display(channel);
            break;
        }

        case MESSAGE_SET_LED_CHAN0:
        case MESSAGE_SET_LED_CHAN1:
        case MESSAGE_SET_LED_CHAN2:
        case MESSAGE_SET_LED_CHAN3:
        {
            uint8_t led_chan = msg.msg8[0] - MESSAGE_SET_LED_CHAN0;
            if (led_chan > 3)
            {
                printf("Unexpected led chan id of %d\n", led_chan);
            }

            LED led;
            switch(led_chan)
            {
                case 0: led = LED::Channel1; break;
                case 1: led = LED::Channel2; break;
                case 2: led = LED::Channel3; break;
                case 3: led = LED::Channel4; break;
                default: return;
            }

            uint8_t red   = msg.msg8[1];
            uint8_t green = msg.msg8[2];
            uint8_t blue  = msg.msg8[3];

            uint32_t colour = blue | (green << 8) | (red << 16); 
            _led_control->set_led_colour(led, colour);
            break;
        }

        case MESSAGE_SET_ACC_IO_PORT1_STATE:
            set_acc_io_port_state(ExtInputPort::ACC_IO_1, msg.msg8[1]);
            break;

        case MESSAGE_SET_ACC_IO_PORT2_STATE:
            set_acc_io_port_state(ExtInputPort::ACC_IO_2, msg.msg8[1]);
            break;

        case MESSAGE_SET_ACC_IO_PORT3_STATE:
            set_acc_io_port_state(ExtInputPort::ACC_IO_3, msg.msg8[1]);
            break;

        case MESSAGE_SET_ACC_IO_PORT_RESET:
            reset_acc_port();
            break;
    }
}

void CRoutineOutputCore1::collar_transmit (uint16_t id, CCollarComms::collar_channel channel, CCollarComms::collar_mode mode, uint8_t power)
{
    // Shove the collar message in global variable protected by a mutex, then send 
    // a message on the FIFO queue to tell core1 there's a message to be sent
    if (multicore_fifo_wready())
    {
        if (mutex_enter_timeout_ms(&g_collar_message_mutex, 100))
        {
            g_collar_message.channel = channel;
            g_collar_message.mode = mode;
            g_collar_message.id = id;
            g_collar_message.power = power;

            message msg = {0};
            msg.msg8[0] = MESSAGE_TRIGGER_COLLAR;
            msg.msg8[1] = 0;
            msg.msg8[2] = 0;
            msg.msg8[3] = 0;

            mutex_exit(&g_collar_message_mutex);
            multicore_fifo_push_blocking(msg.msg32);       
        }
        else
        {
            printf("collar_transmit(): Unable to obtain mutex\n");
        }
    }
    else
    {
        printf("collar_transmit(): FIFO queue to core1 full\n");
    }
}

void CRoutineOutputCore1::reinit_channels()
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_REINIT_CHANNELS;
    msg.msg8[1] = 0;
    msg.msg8[2] = 0;
    msg.msg8[3] = 0;

    multicore_fifo_push_blocking(msg.msg32);
}

void CRoutineOutputCore1::audio_threshold_reached(uint16_t fundamental_freq, uint8_t cross_count)
{
    message msg = {0};
    msg.msg8[0] = MESSAGE_AUDIO_THRES_REACHED;
    msg.msg8[1] = fundamental_freq & 0xFF;
    msg.msg8[2] = (fundamental_freq >> 8) & 0xFF;
    msg.msg8[3] = cross_count;

    multicore_fifo_push_blocking(msg.msg32);
}

void CRoutineOutputCore1::reset_acc_port()
{
    if (*_ext_port_exp)
        (*_ext_port_exp)->reset_acc_port();
}

void CRoutineOutputCore1::set_acc_io_port_state(ExtInputPort output, bool high)
{
    if (*_ext_port_exp)
        (*_ext_port_exp)->set_acc_io_port_state(output, high);
}
