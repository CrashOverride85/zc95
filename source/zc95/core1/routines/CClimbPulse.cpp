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

#include "CClimbPulse.h"
#include "../config.h"
#include <string.h>

#define DEFAULT_CLIMB_TIME_SEC 60
#define DEFAULT_PULSE_DURATION_MS 1000

enum menu_ids
{
    CLIMB_DURATION = 1,
    CLIMB_RESET = 2,
    CLIMB_PULSE_DURATION = 3
};

CClimbPulse::CClimbPulse(uint8_t param)
{
    printf("CClimbPulse()\n");
    _next_pulse_time = 0;
    _pulse_gap_us = _inital_pulse_gap_us;
    _final_pulse_duration_ms = DEFAULT_PULSE_DURATION_MS;
}

CClimbPulse::~CClimbPulse()
{
    printf("~CClimbPulse()\n");
}

void CClimbPulse::get_config(struct routine_conf *conf)
{
    CClimbPulse::config(conf);
}

void CClimbPulse::config(struct routine_conf *conf)
{
    conf->name = "Climb with pulse";
    conf->button_text[(int)soft_button::BUTTON_A] = "Reset";

    // Want four output channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Duration" - a min/max entry in seconds 
    struct menu_entry duration = new_menu_entry();
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
    struct menu_entry reset_after_climb = new_menu_entry();
    reset_after_climb.id = menu_ids::CLIMB_RESET;
    reset_after_climb.title = "Reset after climb";
    reset_after_climb.menu_type = menu_entry_type::MULTI_CHOICE;
    reset_after_climb.multichoice.current_selection = 0;
    reset_after_climb.multichoice.choices.push_back(get_choice("Yes", 1));
    reset_after_climb.multichoice.choices.push_back(get_choice("No", 0));
    conf->menu.push_back(reset_after_climb);

    // menu entry 3: "Pulse Duration" - a min/max entry in ms 
    struct menu_entry pulse_duration = new_menu_entry();
    pulse_duration.id = menu_ids::CLIMB_PULSE_DURATION;
    pulse_duration.title = "Pulse duration";
    pulse_duration.menu_type = menu_entry_type::MIN_MAX;
    pulse_duration.minmax.UoM = "ms";
    pulse_duration.minmax.increment_step = 50;
    pulse_duration.minmax.min = 100;
    pulse_duration.minmax.max = 5000;
    pulse_duration.minmax.current_value = DEFAULT_PULSE_DURATION_MS;
    conf->menu.push_back(pulse_duration);    
}

void CClimbPulse::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    switch (menu_id)
    {
        case menu_ids::CLIMB_DURATION:
            set_pulse_step_from_duration_seconds(new_value);
            break;

        case menu_ids::CLIMB_PULSE_DURATION:
            _final_pulse_duration_ms = new_value;
            break;
    }
}

void CClimbPulse::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    switch (menu_id)
    {
        case menu_ids::CLIMB_RESET:
            _reset_after_climb = choice_id;
            break;
    }
}

void CClimbPulse::soft_button_pushed (soft_button button, bool pushed)
{
    if (pushed && button == soft_button::BUTTON_A)
    {
        _reset = true;
    }
}

void CClimbPulse::start()
{   
    // Match the power set on front panel, which unless the dials are set to max, isn't the full power the box can ma
    set_all_channels_power(POWER_FULL);
    
    _pulse_gap_us = _inital_pulse_gap_us;
    _next_pulse_time = 0;
    set_pulse_step_from_duration_seconds(DEFAULT_CLIMB_TIME_SEC);
    _final_pulse_duration_ms = DEFAULT_PULSE_DURATION_MS;
}

void CClimbPulse::set_pulse_step_from_duration_seconds(uint16_t duration_sec)
{
    _pulse_step_us = 1800 / duration_sec;
}

void CClimbPulse::reset(uint64_t time_us)
{
    simple_channel_off(2);
    simple_channel_off(3);
    _pulse_gap_us = _inital_pulse_gap_us;
    _final_pulse_start_us = 0;
    _reset = false;
}

void CClimbPulse::loop(uint64_t time_us)
{
    if (_reset)
    {
        reset(time_us);
    }

    if (_final_pulse_start_us)
    {
        if (_reset_after_climb && (time_us > (_final_pulse_start_us + _final_pulse_duration_ms*1000)))
        {
            reset(time_us);
            return;
        }
    }

    if (time_us > _next_pulse_time)
    {
        for (int chan=0; chan < 2; chan++)
        {
            full_channel_pulse(chan, DEFAULT_PULSE_WIDTH, DEFAULT_PULSE_WIDTH);
        }

        if (!_final_pulse_start_us)
        {
            _pulse_gap_us -= _pulse_step_us;
            if (_pulse_gap_us < 5000)
            {
                _pulse_gap_us = 5000;

                _final_pulse_start_us = time_us;
                simple_channel_on(2);
                simple_channel_on(3);
            }
        }

        _next_pulse_time = time_us + _pulse_gap_us;
    }
}

void CClimbPulse::stop()
{
    simple_channel_off(2);
    simple_channel_off(3);
}
