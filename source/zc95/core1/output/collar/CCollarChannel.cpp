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

#include "CCollarChannel.h"

/*
 * Allow use of 433MHz shock collars as a channel
 */

// TODO: keepalive();


CCollarChannel::CCollarChannel(
    CSavedSettings *saved_settings, 
    CCollarComms *comms, 
    CPowerLevelControl *power_level_control, 
    uint8_t channel_id) :
    CSimpleOutputChannel(saved_settings, power_level_control, channel_id)
{
    printf("CCollarChannel(%d)\n", channel_id);
    _comms = comms;
    _current_status = collar_status::OFF;
    _last_tx_time_us = 0;
    _led_off_time = 0;
    _channel_id = channel_id;
    _collar_level = 0;

    saved_settings->get_collar_config(channel_id, _collar_conf);

    _inital_led_colour = LedColour::Yellow;
    set_led_colour(_inital_led_colour);
}

CCollarChannel::~CCollarChannel()
{
    printf("~CCollarChannel(%d)\n", _channel_id);
    set_led_colour(LedColour::Black);
}

void CCollarChannel::set_absolute_power(uint16_t power)
{
    // Power requested is 0-1000, but collar expects 0-99, so scale
    set_collar_level_from_power(power);
}

void CCollarChannel::on()
{
    _current_status = collar_status::ON;
    _pulse_end_time = 0; // sent an On instruction, so don't switch off until off is called
    transmit( _collar_level);
}

void CCollarChannel::pulse(uint16_t minimum_duration_ms)
{
    if (minimum_duration_ms < 300)
    {
        _pulse_end_time = 0;
        transmit( _collar_level);
        _current_status = collar_status::OFF;
    }
    else
    {
        _pulse_end_time = get_time_us() + (minimum_duration_ms * 1000);
        transmit(_collar_level);
        _current_status = collar_status::ON;
    }
}

void CCollarChannel::off()
{
    _current_status = collar_status::OFF;
    _pulse_end_time = 0; 
}

void CCollarChannel::loop(uint64_t time_us)
{
    if (_pulse_end_time)
    {
        if (time_us > _pulse_end_time)
        {
            off();
        }
    }

    // If "On", transmit on code every 350 ms
    if (_current_status == collar_status::ON)
    {
        if (time_us - _last_tx_time_us > (350 * 1000))
        {
            transmit(_collar_level);
        }
    }

    if (_led_off_time)
    {
        if (get_time_us() > _led_off_time)
        {
            _led_off_time = 0;
            set_led_colour(LedColour::Yellow);
        }
    }
}

void CCollarChannel::set_collar_level_from_power(int16_t power)
{
    float scaled_power = (float)power * 0.099;
    _collar_level = ceil(scaled_power);
    if (_collar_level > 99)
        _collar_level = 99;

    printf("collar_level = %d (col %d)\n", _collar_level, _channel_id);
}

void CCollarChannel::transmit (uint8_t power)
{
    _led_off_time = get_time_us() + (400 * 1000);
    set_led_colour(LedColour::Red);

     // If we've already transmitted something in the last 100ms, skip this transmission
    if (get_time_us() -_last_tx_time_us < (100 * 1000))
        return;

    _comms->transmit(_collar_conf.id, (CCollarComms::collar_channel)_collar_conf.channel, (CCollarComms::collar_mode)_collar_conf.mode, _collar_level);
    _last_tx_time_us = get_time_us();
}
