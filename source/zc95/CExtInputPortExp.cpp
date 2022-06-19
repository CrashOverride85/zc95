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

#include "CExtInputPortExp.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include <string.h>

/*
 * Deal with port expander U8, which is connected to:
 *   - GPIO1/2/3 on the accessory port
 *   - Trigger1/2 sockets - each socket is "stereo", and is connected to two I/O lines to allow for dual inputs per socket
 */ 

CExtInputPortExp::CExtInputPortExp(uint8_t address, CLedControl *led, CRoutineOutput *routine_output)
{
    _address = address;
    _old_state = 3;
    _last_read = 0;    
    _input_states_at_last_check = 0;
    _led = led;
    _interrupt = false;
    _routine_output = routine_output;
    _debounce_recheck_time_us = 0;
    memset(_input_last_change_time_us, 0, sizeof(_input_last_change_time_us));
}

// Clear any pending input. has_input_been_pressed, etc., should return false after this has been called.
// Can't do this in the constructor as i2c won't have been initialised 
void CExtInputPortExp::clear_input()
{
    int retval = i2c_read_timeout_us(i2c_default, _address, &_last_read, 1, false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CExtInputPortExp::clear_intput i2c read error!\n");
    }

    _input_states_at_last_check = _last_read;
    update_trigger_leds();
}

void CExtInputPortExp::interrupt()
{
    _interrupt = true;
}

void CExtInputPortExp::process(bool force_update)
{
    if (_debounce_recheck_time_us > 0 && time_us_64() > _debounce_recheck_time_us)
    {
        force_update = true;
        _debounce_recheck_time_us = 0;
    }

    if (_interrupt || force_update)
    {
        _interrupt = false;
        uint8_t buffer[1];
        
        int retval = i2c_read_timeout_us(i2c_default, _address, buffer, 1, false, 1000);
        if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
        {
            printf("CExtInputPortExp::process i2c read error!\n");
            _interrupt = true;
            return;
        }

        if ((buffer[0] != _last_read) || force_update)
        {
            _last_read = buffer[0];
            update_trigger_leds();
            update_active_routine();
        }
    }
}

bool CExtInputPortExp::input_state(enum ExtInputPort input)
{
    return (_last_read & (1 << (uint8_t)input));
}

bool CExtInputPortExp::has_input_state_changed(enum ExtInputPort input, bool *new_state)
{
    bool input_state_changed;
    bool last_input_state = (_input_states_at_last_check & (1 << (uint8_t)input));
    *new_state = input_state(input);

    input_state_changed = (last_input_state != *new_state);
    
    if (!input_state_changed)
        return false;

    // Debounce input
    if (input_state_changed)
    {
        if (time_us_64() - _input_last_change_time_us[(int)input] < TRIGGER_INPUT_DEBOUNCE_US)
        {
            _input_last_change_time_us[(int)input] = time_us_64();
            _debounce_recheck_time_us = time_us_64() + TRIGGER_INPUT_DEBOUNCE_US;
            return false;
        }

        _input_last_change_time_us[(int)input] = time_us_64();
    }

    if (*new_state)
        _input_states_at_last_check |= (1 << (uint8_t)input);
    else
        _input_states_at_last_check &= ~(1 << (uint8_t)input);

    return input_state_changed;
}

void CExtInputPortExp::update_trigger_leds()
{
    update_led_for_trigger_port(Trigger::Trigger1);
    update_led_for_trigger_port(Trigger::Trigger2);
}

void CExtInputPortExp::update_led_for_trigger_port(Trigger trigger)
{
    ExtInputPort input_a;
    ExtInputPort input_b;
    LED led;

    switch(trigger)
    {
        case Trigger::Trigger1:
            input_a = ExtInputPort::TRG1_A;
            input_b = ExtInputPort::TRG1_B;
            led = LED::Trigger1;
            break;

        case Trigger::Trigger2:
            input_a = ExtInputPort::TRG2_A;
            input_b = ExtInputPort::TRG2_B;
            led = LED::Trigger2;
            break;

        default:
            return;
    }

    bool partA_active = (input_state(input_a) == 0);
    bool partB_active = (input_state(input_b) == 0);

    if (partA_active && partB_active)
        _led->set_led_colour(led, LedColour::Yellow);
    else if (partA_active)
        _led->set_led_colour(led, LedColour::Green);
    else if (partB_active)
        _led->set_led_colour(led, LedColour::Red);
    else
        _led->set_led_colour(led, LedColour::Black);

    //if (trigger == Trigger::Trigger1)
    //    printf("trg=%d, a=%d, b=%d\n", (uint8_t)trigger, input_state(input_a) , input_state(input_b) );
}

void CExtInputPortExp::update_active_routine()
{
    update_active_routine_trigger(ExtInputPort::TRG1_A, trigger_socket::Trigger1, trigger_part::A);
    update_active_routine_trigger(ExtInputPort::TRG1_B, trigger_socket::Trigger1, trigger_part::B);
    update_active_routine_trigger(ExtInputPort::TRG2_A, trigger_socket::Trigger2, trigger_part::A);
    update_active_routine_trigger(ExtInputPort::TRG2_B, trigger_socket::Trigger2, trigger_part::B);
}

void CExtInputPortExp::update_active_routine_trigger(enum ExtInputPort input, trigger_socket socket, trigger_part part)
{
    bool state;
    if (has_input_state_changed(input, &state))
    {
        // At this point, becasue there are pull ups on the input lines and trigging one gounds it, a 1/high read from the 
        // the port expander means a trigger isn't active (or just isn't plugged in).
        // A low means it's been triggered (button pushed, or whatever)
        // But for routines, it makes far more sense for that to be flipped, i.e. active=true means pressed/triggered
        _routine_output->trigger(socket, part, !state);
    }
}

void CExtInputPortExp::reset_acc_port()
{
    _output_mask = 0xFF;

    int retval = i2c_write_timeout_us(i2c_default, _address, &_output_mask, 1, false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CExtInputPortExp::set_acc_io_port_state i2c write error!\n");
    }
}

void CExtInputPortExp::set_acc_io_port_state(enum ExtInputPort output, bool high)
{
    switch (output)
    {
        case ExtInputPort::ACC_IO_1:
        case ExtInputPort::ACC_IO_2:
        case ExtInputPort::ACC_IO_3:
            if (high)
                _output_mask |= (1 << (uint8_t)output);
            else
                _output_mask &= ~(1 << (uint8_t)output);
        
            break;
        
        default:
            return;
    }

    int retval = i2c_write_timeout_us(i2c_default, _address, &_output_mask, 1, false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CExtInputPortExp::set_acc_io_port_state i2c write error!\n");
    }
}
