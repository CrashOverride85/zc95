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

#include "CAudioIntensity.h"

enum menu_ids
{
    AUDIO_INTENSITY_STEREO = 1,
    AUDIO_INTENSITY_MONO   = 2
};

#define CHANNEL_COUNT 4


CAudioIntensity::CAudioIntensity(uint8_t param)
{
    printf("CAudioIntensity()\n");
    _mono = true;
}

CAudioIntensity::~CAudioIntensity()
{
    printf("~CAudioIntensity()\n");
}

void CAudioIntensity::config(struct routine_conf *conf)
{
    conf->name = "Audio Intensity";

    // Want 4x full channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    conf->audio_processing_mode = audio_mode_t::AUDIO_INTENSITY;

    struct menu_entry menu_mono = new_menu_entry();
    menu_mono.id = menu_ids::AUDIO_INTENSITY_MONO;
    menu_mono.title = "Mono";
    menu_mono.menu_type = menu_entry_type::AUDIO_VIEW_INTENSITY_MONO;
    conf->menu.push_back(menu_mono);

    struct menu_entry menu_stereo = new_menu_entry();
    menu_stereo.id = menu_ids::AUDIO_INTENSITY_STEREO;
    menu_stereo.title = "Stereo";
    menu_stereo.menu_type = menu_entry_type::AUDIO_VIEW_INTENSITY_STEREO;
    conf->menu.push_back(menu_stereo);
}

void CAudioIntensity::get_config(struct routine_conf *conf)
{
   CAudioIntensity::config(conf);
}

void CAudioIntensity::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{

}

void CAudioIntensity::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    

}

void CAudioIntensity::menu_selected(uint8_t menu_id)
{
    if (menu_id == menu_ids::AUDIO_INTENSITY_MONO)
        _mono = true;
    else
        _mono = false;
}

void CAudioIntensity::soft_button_pushed (soft_button button, bool pushed)
{
   
}

void CAudioIntensity::trigger(trigger_socket socket, trigger_part part, bool active)
{

}

void CAudioIntensity::audio_intensity(uint8_t left_chan, uint8_t right_chan, uint8_t virt_chan)
{
    uint16_t power_left  = left_chan  * 4;
    uint16_t power_right = right_chan * 4;
    if (power_left > 1000)
        power_left = 1000;

    if (power_right > 1000)
        power_right = 1000;

    if (_mono)
    {
        full_channel_set_power(0, power_left);
        full_channel_set_power(1, power_left);
        full_channel_set_power(2, power_left);
        full_channel_set_power(3, power_left);        
    }
    else
    {
        full_channel_set_power(0, power_left );
        full_channel_set_power(1, power_left );
        full_channel_set_power(2, power_right);
        full_channel_set_power(3, power_right);
    }
}

void CAudioIntensity::start()
{
    set_all_channels_power(0);
    
    full_channel_on(0);
    full_channel_on(1);
    full_channel_on(2);
    full_channel_on(3);
}

void CAudioIntensity::loop(uint64_t time_us)
{

}

void CAudioIntensity::stop()
{

}
