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

#include "CDummyOutput.h"

/*
 * Dummy output for when nothing is configured to use a channel_id
 */

CDummyOutput::CDummyOutput(
    CSavedSettings *saved_settings, 
    CPowerLevelControl *power_level_control, 
    uint8_t channel_id) :
    COutputChannel(saved_settings, power_level_control, channel_id)
{
    printf("CDummyOutput(%d)\n", channel_id);
    _channel_id = channel_id;

    _standby_led_colour = LedColour::Black;
    set_led_colour(_standby_led_colour);
}

CDummyOutput::~CDummyOutput()
{
    printf("~CDummyOutput(%d)\n", _channel_id);
    set_led_colour(LedColour::Black);
}

void CDummyOutput::set_absolute_power(uint16_t power)
{
}

void CDummyOutput::channel_pulse(uint16_t minimum_duration_ms)
{
}

void CDummyOutput::off()
{

}

void CDummyOutput::loop(uint64_t time_us)
{

}

CChannel_types::channel_type CDummyOutput::get_channel_type()
{
    return CChannel_types::channel_type::CHANNEL_NONE; 
}
