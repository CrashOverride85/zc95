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

/* Process pulses or freq/pulse width requests being sent to us remotely.
 * This isn't displayed on the menu, but is started when in certain remote access modes
 */


CDirectPulse::CDirectPulse(uint8_t param)
{
    printf("CDirectPulse()\n");

    for(uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        _chan_pos_pulse_width[chan] = DEFAULT_PULSE_WIDTH;
        _chan_neg_pulse_width[chan] = DEFAULT_PULSE_WIDTH;
    }
}

CDirectPulse::~CDirectPulse()
{
    printf("~CDirectPulse()\n");
}

void CDirectPulse::config(struct routine_conf *conf)
{
    conf->name = "DirectControl";

    // Want 4x full channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    // menu_id's 0-3 are for channel 0-3 power
    struct menu_entry menu_chan_power[MAX_CHANNELS] = {0};
    for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        menu_chan_power[chan].id = chan;
        menu_chan_power[chan].title = "Chan " + std::to_string(chan+1) + " power";
        menu_chan_power[chan].menu_type = menu_entry_type::MIN_MAX;
        menu_chan_power[chan].minmax.UoM = "";
        menu_chan_power[chan].minmax.increment_step = 1;
        menu_chan_power[chan].minmax.min = 0;
        menu_chan_power[chan].minmax.max = 1000;
        menu_chan_power[chan].minmax.current_value = 0;
        conf->menu.push_back(menu_chan_power[chan]);
    }

    // menu_id's 10-13 are for channel 0-3 positive pulse width
    struct menu_entry menu_chan_pos_pulse_width[MAX_CHANNELS] = {0};
    for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        menu_chan_pos_pulse_width[chan].id = chan+10;
        menu_chan_pos_pulse_width[chan].title = "Chan " + std::to_string(chan+1) + " +pulse wid";
        menu_chan_pos_pulse_width[chan].menu_type = menu_entry_type::MIN_MAX;
        menu_chan_pos_pulse_width[chan].minmax.UoM = "us";
        menu_chan_pos_pulse_width[chan].minmax.increment_step = 1;
        menu_chan_pos_pulse_width[chan].minmax.min = 0;
        menu_chan_pos_pulse_width[chan].minmax.max = 255;
        menu_chan_pos_pulse_width[chan].minmax.current_value = DEFAULT_PULSE_WIDTH;
        conf->menu.push_back(menu_chan_pos_pulse_width[chan]);
    }

    // menu_id's 20-23 are for channel 0-3 negative pulse width
    struct menu_entry menu_chan_neg_width[MAX_CHANNELS] = {0};
    for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        menu_chan_neg_width[chan].id = chan+20;
        menu_chan_neg_width[chan].title = "Chan " + std::to_string(chan+1) + " -pulse wid";
        menu_chan_neg_width[chan].menu_type = menu_entry_type::MIN_MAX;
        menu_chan_neg_width[chan].minmax.UoM = "us";
        menu_chan_neg_width[chan].minmax.increment_step = 1;
        menu_chan_neg_width[chan].minmax.min = 0;
        menu_chan_neg_width[chan].minmax.max = 255;
        menu_chan_neg_width[chan].minmax.current_value = DEFAULT_PULSE_WIDTH;
        conf->menu.push_back(menu_chan_neg_width[chan]);
    }

    // menu_id's 30-33 are for channel 0-3 frequency (hz)
    struct menu_entry menu_chan_freq[MAX_CHANNELS] = {0};
    for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        menu_chan_freq[chan].id = chan+30;
        menu_chan_freq[chan].title = "Chan " + std::to_string(chan+1) + " freq";
        menu_chan_freq[chan].menu_type = menu_entry_type::MIN_MAX;
        menu_chan_freq[chan].minmax.UoM = "hz";
        menu_chan_freq[chan].minmax.increment_step = 1;
        menu_chan_freq[chan].minmax.min = 5;
        menu_chan_freq[chan].minmax.max = 250;
        menu_chan_freq[chan].minmax.current_value = DEFAULT_FREQ_HZ;
        conf->menu.push_back(menu_chan_freq[chan]);
    }

    // menu_id's 40-43 are for channel 0-3 power enable (true/false)
    struct menu_entry menu_chan_power_enable[MAX_CHANNELS] = {0};
    for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        menu_chan_power_enable[chan].id = chan+40;
        menu_chan_power_enable[chan].title = "Chan " + std::to_string(chan+1) + " power";
        menu_chan_power_enable[chan].menu_type = menu_entry_type::MULTI_CHOICE;
        menu_chan_power_enable[chan].multichoice.current_selection = 0;
        menu_chan_power_enable[chan].multichoice.choices.push_back(get_choice("Off", 0));
        menu_chan_power_enable[chan].multichoice.choices.push_back(get_choice("On", 1));
        conf->menu.push_back(menu_chan_power_enable[chan]);
    }

    struct menu_entry menu_chan_iso = new_menu_entry();
    menu_chan_iso.id = 100;
    menu_chan_iso.title = "Chan isolation";
    menu_chan_iso.menu_type = menu_entry_type::MULTI_CHOICE;
    menu_chan_iso.multichoice.current_selection = 1;
    menu_chan_iso.multichoice.choices.push_back(get_choice("On", 1));
    menu_chan_iso.multichoice.choices.push_back(get_choice("Off", 0));
    conf->menu.push_back(menu_chan_iso);

    conf->audio_processing_mode = audio_mode_t::OFF;
    conf->force_channel_isolation = false;
    conf->hidden_from_menu = true;
}

void CDirectPulse::get_config(struct routine_conf *conf)
{
   CDirectPulse::config(conf);
}

void CDirectPulse::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    for(uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        // power level
        if (menu_id == chan)
        {
            if (new_value < 0 || new_value > 1000)
                return;

            full_channel_set_power(menu_id, new_value);
            _chan_last_power_level[chan] = new_value;
            return;
        }

        // positive pulse width
        else if (menu_id == chan+10)
        {
            if (new_value < 0 || new_value > 255)
                return;
            
            _chan_pos_pulse_width[chan] = new_value;

            full_channel_set_pulse_width(chan, _chan_pos_pulse_width[chan], _chan_neg_pulse_width[chan]);
            return;
        }

        // negative pulse width
        else if (menu_id == chan+20)
        {
            if (new_value < 0 || new_value > 255)
                return;
            
            _chan_neg_pulse_width[chan] = new_value;

            full_channel_set_pulse_width(chan, _chan_pos_pulse_width[chan], _chan_neg_pulse_width[chan]);
            return;
        }

        // frequency
        else if (menu_id == chan+30)
        {
            if (new_value < 1 || new_value > 255)
                return;
            
            full_channel_set_freq(chan, new_value);
            return;
        }
    }
}

void CDirectPulse::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    // Channel isolation
    if (menu_id == 100)
    {
        printf("chan iso change\n");

        if (choice_id == 0)
            set_channel_isolation(false);
        else
            set_channel_isolation(true);

        return;
    }

    for(uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        // power enable/disable
        if (menu_id == chan+40)
        {
            if (choice_id == 1)
                full_channel_on(chan);
            else
                full_channel_off(chan);

            return;
        }
    }
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

    for(uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
        _chan_last_power_level[chan] = 0;
}

void CDirectPulse::loop(uint64_t time_us)
{

}

void CDirectPulse::stop()
{
    set_all_channels_power(0);
}
