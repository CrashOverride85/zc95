/*
 * ZC95
 * Copyright (C) 2022  CrashOverride85
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

#include "CAudioThreshold.h"

enum menu_ids
{
    AUDIO_VIEW = 1
};

#define CHANNEL_COUNT 4


CAudioThreshold::CAudioThreshold()
{
    printf("CAudioThreshold()\n");

}

CAudioThreshold::~CAudioThreshold()
{
    printf("~CAudioThreshold()\n");
}

void CAudioThreshold::config(struct routine_conf *conf)
{
    conf->name = "Audio Threshold";

    // Want 4x simple channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Audio trigger"
    struct menu_entry menu_audio;
    menu_audio.id = menu_ids::AUDIO_VIEW;
    menu_audio.title = "Audio trigger";
    menu_audio.menu_type = menu_entry_type::AUDIO_VIEW;
    menu_audio.audioview.default_trigger_position = 0;
    conf->menu.push_back(menu_audio);
}

void CAudioThreshold::get_config(struct routine_conf *conf)
{
   CAudioThreshold::config(conf);
}

void CAudioThreshold::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{

}

void CAudioThreshold::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    

}

void CAudioThreshold::soft_button_pushed (soft_button button, bool pushed)
{
   
}

void CAudioThreshold::trigger(trigger_socket socket, trigger_part part, bool active)
{

}

void CAudioThreshold::audio_threshold_reached(uint16_t fundamental_freq, uint8_t cross_count)
{
    uint16_t duration_ms = cross_count * 10;
    if (duration_ms > 500)
        duration_ms = 500;

    if (duration_ms < 21)
        duration_ms = 21;
    all_channels_pulse(duration_ms);
}

void CAudioThreshold::start()
{
    set_all_channels_power(POWER_FULL);
}

void CAudioThreshold::loop(uint64_t time_us)
{

}

void CAudioThreshold::stop()
{

}

void CAudioThreshold::all_channels(bool on)
{

}

void CAudioThreshold::all_channels_pulse(uint16_t pulse_len_ms)
{
    for (int x=0; x < CHANNEL_COUNT; x++)   
    {
        simple_channel_pulse(x, pulse_len_ms);
    }
}
