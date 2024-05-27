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
/*
    struct menu_entry menu_chan[MAX_CHANNELS] = {0};

    for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        menu_chan[chan].id = chan;
        menu_chan[chan].title = "Chan " + std::to_string(chan+1) + " power";
        menu_chan[chan].menu_type = menu_entry_type::MIN_MAX;
        menu_chan[chan].minmax.UoM = "";
        menu_chan[chan].minmax.increment_step = 1;
        menu_chan[chan].minmax.min = 0;
        menu_chan[chan].minmax.max = 1000;
        menu_chan[chan].minmax.current_value = 0;
        conf->menu.push_back(menu_chan[chan]);
    }
*/
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
/*
    if (menu_id >= MAX_CHANNELS)
        return;

    if (new_value < 0)
        new_value = 0;

    if (new_value > 1000)
        new_value = 1000;

    full_channel_set_power(menu_id, new_value);
    */
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


void CDirectPulse::pulse_message(uint8_t channel, uint16_t power_level, uint8_t pos_pulse_us, uint8_t neg_pulse_us)
{
    if (channel < MAX_CHANNELS)
    {
        if (_chan_last_power_level[channel] != power_level)
        {
            full_channel_set_power(channel, power_level);
            _chan_last_power_level[channel] = power_level;
        }

        full_channel_pulse(channel, pos_pulse_us, neg_pulse_us);
    }
}

void CDirectPulse::start()
{
    set_all_channels_power(0);
    for (uint8_t chan=0; chan < MAX_CHANNELS; chan++)
    {
        _chan_last_power_level[chan] = 0;
    }
}

void CDirectPulse::loop(uint64_t time_us)
{

}

void CDirectPulse::stop()
{

}
