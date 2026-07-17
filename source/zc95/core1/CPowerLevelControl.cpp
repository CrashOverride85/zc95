/*
 * ZC95
 * Copyright (C) 2026  CrashOverride85
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

#include "CPowerLevelControl.h"
#include "pico/stdlib.h"
#include <string.h>
#include <cmath>

/*
 * Deal with everything relating to the output power.
 * Includes:
 *   - routine_requested_power - Power level (0-1000) that a routine has requested. It will usually be 1000.
 *   - front_panel_power       - Power level (0-1000) as set on the front panel. For v0.1 of the front panel, there are only 255 discrete values possible
 *   - remote_access_power     - Power level (0-1000) set by remote UI. If in remote access mode, this largely takes the place of front_panel_power,
 *                               with front_panel_power serving as a power limit
 *   - max_power_level         - The maximum power level currently possible - always <= front_panel_power. This takes into account ramp up 
 *                             - time, so for the first few seconds this will increase (assuming the front panel isn't set to 0)
 *   - output_power_level      - The power level to be sent to output chanel after combining all the above. Also scaled based on power mode (high/medium/low)
 */

CPowerLevelControl::CPowerLevelControl(CSavedSettings *saved_settings, uint8_t channel_count) : _channel_count(channel_count), _extended_ramp(saved_settings)
{
    printf("CPowerLevelControl()\n");
    _saved_settings = saved_settings;

    _front_panel_power   = new uint16_t[_channel_count]();
    _remote_access_power = new uint16_t[_channel_count]();
    _routine_power       = new uint16_t[_channel_count]();
    _output_power        = new uint16_t[_channel_count]();

    _initial_ramp_percent = 0; 
    _initial_ramp_last_increment_us = 0;
    _initial_ramp_increment_period_ms = 0;
        
    _initial_ramp_in_progress = false;
    _remote_mode_active = false;
}

CPowerLevelControl::~CPowerLevelControl()
{
    printf("~CPowerLevelControl()\n");
    delete[] _front_panel_power;
    delete[] _remote_access_power;
    delete[] _routine_power;
    delete[] _output_power;
}

// Call with the power level set on the front panel 
// power is 0-1000, channel is 0-3
void CPowerLevelControl::set_front_panel_power(uint8_t channel, uint16_t power)
{
    if (channel >= _channel_count)
        return;

    if (_front_panel_power[channel] != power)
    {
        _front_panel_power[channel] = power;
        calc_output_power(channel);
    }
}

// Call with power level set remotely
// power is 0-1000, channel is 0-3
void CPowerLevelControl::set_remote_power(uint8_t channel, uint16_t power)
{
    if (channel >= _channel_count)
        return;

    if (power > 1000)
        power = 1000;

    if (_remote_access_power[channel] != power)
    {
        _remote_access_power[channel] = power;
        calc_output_power(channel);
    }
}

void CPowerLevelControl::remote_mode_enable()
{
    _remote_mode_active = true;
    for (int chan=0; chan < _channel_count; chan++)
    {
        _remote_access_power[chan] = 0;
        calc_output_power(chan);
    }
}

void CPowerLevelControl::remote_mode_disable()
{
    _remote_mode_active = false;
    for (int chan=0; chan < _channel_count; chan++)
    {
        _remote_access_power[chan] = 0;
        calc_output_power(chan);
    }
    zero_power_level();
}

// Power level being requested by routine (0-1000)
void CPowerLevelControl::set_routine_requested_power_level(uint8_t channel, uint16_t power)
{
    if (channel >= _channel_count)
        return;

    if (_routine_power[channel] != power)
    {
        _routine_power[channel] = power;
        calc_output_power(channel);
    }
}

// Get power level to send to output chanel (0-1000)
uint16_t CPowerLevelControl::get_output_power_level(uint8_t channel)
{
    if (channel >= _channel_count)
        return 0;

    switch(_saved_settings->get_power_level())
    {
        case CSavedSettings::power_level_t::HIGH:
            return _output_power[channel];
        
        case CSavedSettings::power_level_t::MEDIUM:
            return _output_power[channel] * 0.66;
        
        case CSavedSettings::power_level_t::LOW:
            return _output_power[channel] * 0.33;
        
        default:
            return 0;
    }
}

uint16_t CPowerLevelControl::get_display_power_level(uint8_t channel)
{
    if (channel >= _channel_count)
        return 0;

    return _output_power[channel];
}

// Get the current maximum power level (0-1000)
uint16_t CPowerLevelControl::get_max_power_level(uint8_t channel)
{
    uint16_t selected_power = 0;

    if (channel >= _channel_count)
        return 0;

    if (_remote_mode_active)
    {
        // Max power is limited by what's set on the front panel
        selected_power = _remote_access_power[channel];
        if (selected_power > _front_panel_power[channel])
            selected_power = _front_panel_power[channel];
    }
    else
    {
        selected_power = _front_panel_power[channel];
    }

    if (_initial_ramp_in_progress)
        selected_power = (float)selected_power * ((float)_initial_ramp_percent / (float)100);
    
    if (_extended_ramp.ramp_in_progress())
        selected_power = (float)selected_power * (_extended_ramp.get_ramp_power_percent() / (float)100);
    
    return selected_power;
}

// Get the maximum power level (power level set on front panel - 0-1000) that's being ramped up to
uint16_t CPowerLevelControl::get_target_max_power_level(uint8_t channel)
{
    if (channel >= _channel_count)
        return 0;

    return _front_panel_power[channel];
}

void CPowerLevelControl::get_extended_ramp_progress(uint8_t* out_percent, uint16_t* out_secs_remain)
{
    if (!_extended_ramp.ramp_in_progress())
    {
        *out_percent = 0xFF;
        *out_secs_remain = 0xFFFF;
        return;
    }

    float percent_progress = _extended_ramp.get_ramp_progress_percent();

    uint8_t steps = 100 - _saved_settings->get_extended_ramp_level();
    int total_duration_seconds = steps * _saved_settings->get_extended_ramp_time_seconds();

    *out_secs_remain = total_duration_seconds - ((float)total_duration_seconds * (percent_progress / (float)100));
    *out_percent = (uint8_t)percent_progress;
}

void CPowerLevelControl::initial_ramp_start()
{
    // How often should the power level be increased.
    _initial_ramp_increment_period_ms = (_saved_settings->get_initial_ramp_up_time_seconds() * 10);
    
    _initial_ramp_percent = 0;
    _initial_ramp_last_increment_us = 0;
    _initial_ramp_in_progress = true;

    _extended_ramp.reset();
}

void CPowerLevelControl::extended_ramp_start()
{
    _extended_ramp.ramp_start();
}

void CPowerLevelControl::zero_power_level()
{
    _extended_ramp.reset();
    _initial_ramp_in_progress = false;
    _initial_ramp_percent = 0;
    _initial_ramp_last_increment_us = 0;

    for (int chan=0; chan < _channel_count; chan++)
    {
        _routine_power[chan] = 0;
        calc_output_power(chan);
    }
}

void CPowerLevelControl::loop()
{
    bool _recalc_power = false;

    if (_initial_ramp_in_progress)
    {
        if (time_us_64() > _initial_ramp_last_increment_us + (_initial_ramp_increment_period_ms * 1000))
        {
            if (_initial_ramp_percent < 100)
                _initial_ramp_percent++;
            
            if (_initial_ramp_percent == 100)
                _initial_ramp_in_progress = false;

            _initial_ramp_last_increment_us = time_us_64();
            _recalc_power = true;
        }
    }

    _recalc_power |= _extended_ramp.loop();

    if (_recalc_power)
    {
        for (int chan=0; chan < _channel_count; chan++)
        {
            calc_output_power(chan);
        }
    }
}

void CPowerLevelControl::calc_output_power(uint8_t channel)
{
    float scaled_power;

    // Routines request power levels between 0-1000, combine this with the 0-1000 power level either set on 
    // the front panel, or supplied by remote access, to get the actual power required from the channel.
    if (_remote_mode_active)
    {
        scaled_power = (float)_routine_power[channel] * ((float)_remote_access_power[channel] / (float)1000);

        // in remote access mode, the front panel power is used as a power limit
        if (scaled_power > _front_panel_power[channel]) 
            scaled_power = _front_panel_power[channel]; 
    }
    else
    {
        scaled_power = (float)_routine_power[channel] * ((float)_front_panel_power[channel] / (float)1000);
    }
    
    scaled_power = ceil(scaled_power);
    
    if (scaled_power > 1000)
        scaled_power = 1000;

    _output_power[channel] = scaled_power 
                * ((float)_initial_ramp_percent / (float)100)
                * (_extended_ramp.get_ramp_power_percent() / (float)100);
}
