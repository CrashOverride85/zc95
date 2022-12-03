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

#include "CClimb.h"
#include "../config.h"
#include <string.h>

#define DEFAULT_CLIMB_TIME_SEC 60

enum menu_ids
{
    CLIMB_DURATION = 1,
    CLIMB_RESET = 2
};

CClimb::CClimb(uint8_t param)
{
    printf("CClimb()\n");
    _next_pulse_time = 0;
    _pulse_gap_us = _inital_pulse_gap_us;
}

CClimb::~CClimb()
{
    printf("~CClimb()\n");
}

void CClimb::get_config(struct routine_conf *conf)
{
    CClimb::config(conf);
}

void CClimb::config(struct routine_conf *conf)
{
    conf->name = "Climb";
    conf->button_text[(int)soft_button::BUTTON_A] = "Reset";

    // Want four output channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    // menu entry 1: "Duration" - a min/max entry in seconds 
    struct menu_entry duration;
    duration.id = menu_ids::CLIMB_DURATION;
    duration.title = "Duration";
    duration.menu_type = menu_entry_type::MIN_MAX;
    duration.minmax.UoM = "sec";
    duration.minmax.increment_step = 5;
    duration.minmax.min = 5;
    duration.minmax.max = 120;
    duration.minmax.current_value = DEFAULT_CLIMB_TIME_SEC;
    conf->menu.push_back(duration);

    // menu entry 2: "Reset after climb" - when reaching full power, start over or not?
    struct menu_entry reset_after_climb;
    reset_after_climb.id = menu_ids::CLIMB_RESET;
    reset_after_climb.title = "Reset after climb";
    reset_after_climb.menu_type = menu_entry_type::MULTI_CHOICE;
    reset_after_climb.multichoice.current_selection = 0;
    reset_after_climb.multichoice.choices.push_back(get_choice("Yes", 1));
    reset_after_climb.multichoice.choices.push_back(get_choice("No", 0));

    conf->menu.push_back(reset_after_climb);
}

void CClimb::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    switch (menu_id)
    {
        case menu_ids::CLIMB_DURATION:
            set_pulse_step_from_duration_seconds(new_value);
            break;
    }
}

void CClimb::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    switch (menu_id)
    {
        case menu_ids::CLIMB_RESET:
            _reset_after_climb = choice_id;
            break;
    }
}

void CClimb::soft_button_pushed (soft_button button, bool pushed)
{
    if (pushed && button == soft_button::BUTTON_A)
    {
        _reset = true;
    }
}

void CClimb::start()
{   
    // Match the power set on front panel, which unless the dials are set to max, isn't the full power the box can ma
    set_all_channels_power(POWER_FULL);
    
    _pulse_gap_us = _inital_pulse_gap_us;
    _next_pulse_time = 0;
    set_pulse_step_from_duration_seconds(DEFAULT_CLIMB_TIME_SEC);
}

void CClimb::set_pulse_step_from_duration_seconds(uint16_t duration_sec)
{
    _pulse_step_us = 1800 / duration_sec;
}

void CClimb::loop(uint64_t time_us)
{
    if (time_us > _next_pulse_time)
    {
        for (int chan=0; chan < 4; chan++)
        {
            full_channel_pulse(chan, DEFAULT_PULSE_WIDTH, DEFAULT_PULSE_WIDTH);
        }

        _pulse_gap_us -= _pulse_step_us;
        if ((_pulse_gap_us < 1500) || _reset)
        {
            if (_reset_after_climb || _reset)
            {
                _pulse_gap_us = _inital_pulse_gap_us;
            }
            else
            {
                _pulse_gap_us = 1500;
            }
        }

        _next_pulse_time = time_us + _pulse_gap_us;
        _reset = false;
    }
}

void CClimb::stop()
{
 
}
