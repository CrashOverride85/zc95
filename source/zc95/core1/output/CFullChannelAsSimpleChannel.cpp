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

#include "CFullChannelAsSimpleChannel.h"
#include "../config.h"

/*
 * Allow a "full" output channel, e.g. the ZC624 output board (which expects pulse legnths to passed, etc.) to
 * be treated as a simple channel, which can be turned on/off and pulsed for x milliseconds
 */ 


CFullChannelAsSimpleChannel::CFullChannelAsSimpleChannel(CSavedSettings *saved_settings, CFullOutputChannel *full_channel, uint8_t channel_number, CPowerLevelControl *power_level_control) :
 CSimpleOutputChannel(saved_settings, power_level_control, channel_number) 
{
    printf("CFullChannelAsSimpleChannel()\n");
    _inital_led_colour = LedColour::Green;
    _off_time = 0;
    _full_channel = full_channel;

    _full_channel->set_freq(DEFAULT_FREQ_HZ);
    _full_channel->set_pulse_width(DEFAULT_PULSE_WIDTH, DEFAULT_PULSE_WIDTH);
}

CFullChannelAsSimpleChannel::~CFullChannelAsSimpleChannel()
{
    printf("~CFullChannelAsSimpleChannel\n");
}


//////////// CSimpleOutputChannel stuff ////////////

void CFullChannelAsSimpleChannel::on()
{
    _full_channel->set_led_colour(LedColour::Red);
    _full_channel->on();
}

void CFullChannelAsSimpleChannel::off()
{
    _full_channel->set_led_colour(LedColour::Green);
    _full_channel->off();
}

void CFullChannelAsSimpleChannel::pulse(uint16_t minimum_duration_ms)
{
    on();
    _off_time = get_time_us() + (minimum_duration_ms * 1000);
}

////////////////////////////////////////////////////

void CFullChannelAsSimpleChannel::set_absolute_power(uint16_t power)
{
    _full_channel->set_absolute_power(power);
}

void CFullChannelAsSimpleChannel::loop(uint64_t time_us)
{
    if (_off_time && (time_us > _off_time))
    {
        off();
        _off_time = 0;
    }
}

bool CFullChannelAsSimpleChannel::is_internal()
{
    return _full_channel->is_internal();
}
