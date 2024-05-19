/*
 * ZC95
 * Copyright (C) 2024  CrashOverride85
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

#include "CDirectPulse.h"

#define CHANNEL_COUNT 4

/* Process pulses being sent to us remotely 
 * This isn't displayed on the menu, but is started when in certain remote access modes
 */


CDirectPulse::CDirectPulse(uint8_t param)
{
    printf("CDirectPulse()\n");
}

CDirectPulse::~CDirectPulse()
{
    printf("~CDirectPulse()\n");
}

void CDirectPulse::config(struct routine_conf *conf)
{
    conf->name = "DirectPulse";

    // Want 4x full channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    conf->audio_processing_mode = audio_mode_t::OFF;
    conf->enable_channel_isolation = false;
    conf->hidden_from_menu = true;
}

void CDirectPulse::get_config(struct routine_conf *conf)
{
   CDirectPulse::config(conf);
}

void CDirectPulse::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{

}

void CDirectPulse::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    

}

void CDirectPulse::soft_button_pushed (soft_button button, bool pushed)
{
   
}

void CDirectPulse::trigger(trigger_socket socket, trigger_part part, bool active)
{

}


void CDirectPulse::pulse_message(uint8_t channel, uint8_t pos_pulse_us, uint8_t neg_pulse_us)
{
    if (channel < MAX_CHANNELS)
    {
        full_channel_pulse(channel, pos_pulse_us, neg_pulse_us);
    }
}

void CDirectPulse::start()
{
    set_all_channels_power(1000);
}

void CDirectPulse::loop(uint64_t time_us)
{

}

void CDirectPulse::stop()
{

}
