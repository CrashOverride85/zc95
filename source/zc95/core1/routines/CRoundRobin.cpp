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

#include "CRoundRobin.h"
#include <string.h>

enum menu_ids
{
    DELAY = 1,
    PULSE_CONT = 2
};

const uint16_t inital_speed = 1000;

CRoundRobin::CRoundRobin(uint8_t param)
{
    printf("CRoundRobin()\n");
    _delay_setting_ms = inital_speed;
}

CRoundRobin::~CRoundRobin()
{
    printf("~CRoundRobin()\n");
}

void CRoundRobin::get_config(struct routine_conf *conf)
{
    CRoundRobin::config(conf);
}

void CRoundRobin::config(struct routine_conf *conf)
{
    conf->name = "RoundRobin";

    // Need four output channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Delay" - how long to wait before switching to next channel
    struct menu_entry menu_speed = new_menu_entry();
    menu_speed.id = menu_ids::DELAY;
    menu_speed.title = "Delay";
    menu_speed.menu_type = menu_entry_type::MIN_MAX;
    menu_speed.minmax.UoM = "ms";
    menu_speed.minmax.increment_step = 50;
    menu_speed.minmax.min = 50;
    menu_speed.minmax.max = 4000;
    menu_speed.minmax.current_value = inital_speed;
    conf->menu.push_back(menu_speed);

    // menu entry 2: "pulse/cont." - pulse each channel, or toggle between each?
    struct menu_entry menu_pulse_cont = new_menu_entry();
    menu_pulse_cont.id = menu_ids::PULSE_CONT;
    menu_pulse_cont.title = "Pulse/Cont.";
    menu_pulse_cont.menu_type = menu_entry_type::MULTI_CHOICE;
    menu_pulse_cont.multichoice.current_selection = 0;
    menu_pulse_cont.multichoice.choices.push_back(get_choice("Continuous", 0));
    menu_pulse_cont.multichoice.choices.push_back(get_choice("Pulse", 1));
    conf->menu.push_back(menu_pulse_cont);
}

void CRoundRobin::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    if (menu_id == menu_ids::DELAY)
    {
        _delay_setting_ms = new_value;
    }
}

void CRoundRobin::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    if (menu_id == menu_ids::PULSE_CONT)
    {
        // Switch off before changing mode
        set_all_channels_off();

        _wait_until_us = 0;

        if (choice_id == 1)
            _pulse_mode = true;
        else
            _pulse_mode = false;
    }
}

void CRoundRobin::start()
{
    set_all_channels_power(POWER_FULL);
    _current_active_channel = 0;
    _wait_until_us = 0;
}

void CRoundRobin::loop(uint64_t time_us)
{
    if (time_us > _wait_until_us)
    {
        // switch off current channel
        simple_channel_off(_current_active_channel);

        // select next channel
        _current_active_channel++;
        if (_current_active_channel >= 4)
            _current_active_channel=0;

        // switch on new channel
        if (_pulse_mode)
        {
            simple_channel_pulse(_current_active_channel, 100);
        }
        else
        {
            simple_channel_on(_current_active_channel);
        }

        _wait_until_us = time_us + ((uint64_t)_delay_setting_ms * 1000);
    }
}

void CRoundRobin::stop()
{
    set_all_channels_power(0);
}
