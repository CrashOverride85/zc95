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

#include "CPowerLevelControl.h"
#include "pico/stdlib.h"
#include <string.h>
#include <cmath>

/*
 * Deal with everything relating to the output power.
 * Includes:
 *   - routine_requested_power - Power level (0-1000) that a routine has requested. It will usually be 1000.
 *   - front_panel_power       - Power level (0-1000) as set on the front pannel. Due to the ADC, there are only 255 discrete values possible 
 *   - max_power_level         - The maximum power level currenly possible - always <= front_panel_power. This takes into account ramp up 
 *                             - time, so for the first few seconds this will increase (assuming the front pannel isn't set to 0)
 *   - output_power_level      - The power level to be sent to output chanel after combining all the above
 */

CPowerLevelControl::CPowerLevelControl(CSavedSettings *saved_settings)
{
    _saved_settings = saved_settings;
    memset(_front_panel_power, 0, sizeof(_front_panel_power));
    memset(_routine_power, 0, sizeof(_routine_power));
    memset(_output_power, 0, sizeof(_output_power));

    _ramp_percent = 0; 
    _ramp_last_increment_us = 0;
    _ramp_increment_period_ms = 0;
        
    _ramp_in_progress = false;
}

// Call with the power level set on the front panel 
// power is 0-1000, channel is 0-3
void CPowerLevelControl::set_front_panel_power(uint8_t channel, uint16_t power)
{
    if (channel >= MAX_CHANNELS)
        return;

    if (_front_panel_power[channel] != power)
    {
        _front_panel_power[channel] = power;
        calc_output_power(channel);
    }
}

// Power level being requested by routine (0-1000)
void CPowerLevelControl::set_routine_requested_power_level(uint8_t channel, uint16_t power)
{
    if (channel >= MAX_CHANNELS)
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
    if (channel >= MAX_CHANNELS)
        return 0;

    return _output_power[channel];
}

// Get the current maximum power level (0-1000)
uint16_t CPowerLevelControl::get_max_power_level(uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return 0;

    if (_ramp_in_progress)
        return (float)_front_panel_power[channel] * ((float)_ramp_percent / (float)100);
    else
        return _front_panel_power[channel];
}

// Get the maximum power level (power level set on front pannel - 0-1000) that's being ramped up to
uint16_t CPowerLevelControl::get_target_max_power_level(uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return 0;

    return _front_panel_power[channel];
}

void CPowerLevelControl::ramp_start()
{
    // How often should the power level be increased.
    _ramp_increment_period_ms = (_saved_settings->get_ramp_up_time_seconds() * 10);
    
    _ramp_percent = 0;
    _ramp_last_increment_us = 0;
    _ramp_in_progress = true;
}

void CPowerLevelControl::zero_power_level()
{
    _ramp_in_progress = false;
    _ramp_percent = 0;
    _ramp_last_increment_us = 0;

    for (int chan=0; chan < MAX_CHANNELS; chan++)
    {
        _routine_power[chan] = 0;
        calc_output_power(chan);
    }
}

void CPowerLevelControl::loop()
{
    if (_ramp_in_progress)
    {
        if (time_us_64() > _ramp_last_increment_us + (_ramp_increment_period_ms * 1000))
        {
            if (_ramp_percent < 100)
                _ramp_percent++;
            
            if (_ramp_percent == 100)
                _ramp_in_progress = false;

            _ramp_last_increment_us = time_us_64();
        
            for (int chan=0; chan < MAX_CHANNELS; chan++)
            {
                calc_output_power(chan);
            }
        }
    }
}

void CPowerLevelControl::calc_output_power(uint8_t channel)
{
    // Routines request power levels between 0-1000, combine this with the 0-1000 power level set on 
    // the front pannel to get the actual power required from the channel.
    float scaled_power = (float)_routine_power[channel] * ((float)_front_panel_power[channel] / (float)1000);
    scaled_power = ceil(scaled_power);
    
    if (scaled_power > 1000)
        scaled_power = 1000;

    _output_power[channel] = scaled_power * ((float)_ramp_percent / (float)100);
}
