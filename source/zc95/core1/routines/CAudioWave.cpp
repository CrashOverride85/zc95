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

#include "CAudioWave.h"

enum menu_ids
{
    AUDIO_WAVE = 1
};

#define CHANNEL_COUNT 4


CAudioWave::CAudioWave(uint8_t param)
{
    printf("CAudioWave()\n");
}

CAudioWave::~CAudioWave()
{
    printf("~CAudioWave()\n");
}

void CAudioWave::config(struct routine_conf *conf)
{
    conf->name = "Audio Wave";

    // Want 4x full channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    conf->audio_processing_mode = audio_mode_t::AUDIO3;

    struct menu_entry menu_stereo_view;
    menu_stereo_view.id = menu_ids::AUDIO_WAVE;
    menu_stereo_view.title = "Stereo view";
    menu_stereo_view.menu_type = menu_entry_type::AUDIO_VIEW_WAVE;
    conf->menu.push_back(menu_stereo_view);

    struct menu_entry menu_triphase_view;
    menu_triphase_view.id = menu_ids::AUDIO_WAVE;
    menu_triphase_view.title = "Triphase view";
    menu_triphase_view.menu_type = menu_entry_type::AUDIO_VIEW_VIRTUAL_3;
    conf->menu.push_back(menu_triphase_view);

    conf->enable_channel_isolation = false;
}

void CAudioWave::get_config(struct routine_conf *conf)
{
   CAudioWave::config(conf);
}

void CAudioWave::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{

}

void CAudioWave::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    

}

void CAudioWave::soft_button_pushed (soft_button button, bool pushed)
{
   
}

void CAudioWave::trigger(trigger_socket socket, trigger_part part, bool active)
{

}

void CAudioWave::audio_intensity(uint8_t left_chan, uint8_t right_chan, uint8_t virt_chan)
{
    uint16_t power_left  = left_chan  * 4;
    uint16_t power_right = right_chan * 4;
    if (power_left > 1000)
        power_left = 1000;

    if (power_right > 1000)
        power_right = 1000;

    full_channel_set_power(0, power_left );
    full_channel_set_power(1, power_left );
    full_channel_set_power(2, power_right);
    full_channel_set_power(3, power_right);
}

void CAudioWave::pulse_message(uint8_t channel, uint8_t pos_pulse_us, uint8_t neg_pulse_us)
{
    // Pretty much all the processing for this pattern is done in CAudio/CAudio3Process, and passed into here.
    // All that needs to be done now is copy the 2 channels of data arriving into 4 channels
    if (channel == 0)
    {
        full_channel_pulse(0, pos_pulse_us, neg_pulse_us);
        full_channel_pulse(1, pos_pulse_us, neg_pulse_us);
    }
    else if (channel == 1)
    {
        full_channel_pulse(2, pos_pulse_us, neg_pulse_us);
        full_channel_pulse(3, pos_pulse_us, neg_pulse_us);
    }
}

void CAudioWave::start()
{
    set_all_channels_power(0);
}

void CAudioWave::loop(uint64_t time_us)
{

}

void CAudioWave::stop()
{

}
