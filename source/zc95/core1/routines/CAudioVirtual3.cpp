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

#include "CAudioVirtual3.h"

enum menu_ids
{
    AUDIO_VIRTUAL_3 = 1
};

CAudioVirtual3::CAudioVirtual3()
{
    printf("CAudioVirtual3()\n");
    _mono = true;
}

CAudioVirtual3::~CAudioVirtual3()
{
    printf("~CAudioVirtual3()\n");
}

void CAudioVirtual3::config(struct routine_conf *conf)
{
    conf->name = "Audio virtual 3";

    // Only use 3 channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    conf->audio_processing_mode = audio_mode_t::AUDIO3;

    struct menu_entry menu_mono;
    menu_mono.id = menu_ids::AUDIO_VIRTUAL_3;
    menu_mono.title = "Audio view";
    menu_mono.menu_type = menu_entry_type::AUDIO_VIEW_VIRTUAL_3;
    conf->menu.push_back(menu_mono);
}

void CAudioVirtual3::get_config(struct routine_conf *conf)
{
   CAudioVirtual3::config(conf);
}

void CAudioVirtual3::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{

}

void CAudioVirtual3::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    

}

void CAudioVirtual3::soft_button_pushed (soft_button button, bool pushed)
{
   
}

void CAudioVirtual3::trigger(trigger_socket socket, trigger_part part, bool active)
{

}

void CAudioVirtual3::audio_intensity(uint8_t left_chan, uint8_t right_chan, uint8_t virt_chan)
{
    uint16_t power_left  = left_chan  * 4;
    uint16_t power_right = right_chan * 4;
    uint16_t power_virt  = virt_chan  * 4;

    if (power_left > 1000)
        power_left = 1000;

    if (power_right > 1000)
        power_right = 1000;

    if (power_virt > 1000)
        power_virt = 1000;

    full_channel_set_power(0, power_left );
    full_channel_set_power(1, power_right);
    full_channel_set_power(2, power_virt );
}

void CAudioVirtual3::pulse_message(uint8_t channel, uint8_t pos_pulse_us, uint8_t neg_pulse_us)
{
    // Pretty much all the processing for this pattern is done in CAudio, and passed into here.
    if (channel > 3)
        return;

    full_channel_pulse(channel, pos_pulse_us, neg_pulse_us);
}

void CAudioVirtual3::start()
{
    set_all_channels_power(0);
}

void CAudioVirtual3::loop(uint64_t time_us)
{

}

void CAudioVirtual3::stop()
{

}
