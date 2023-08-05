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

#include "CToggle.h"

enum menu_ids
{
    SPEED = 1,
    PULSE_CONT = 2
};

CToggle::CToggle(uint8_t param)
{
    printf("CToggle()\n");
    _speed_setting = 2000;
}

CToggle::~CToggle()
{
    printf("~CToggle()\n");
}

void CToggle::config(struct routine_conf *conf)
{
    conf->name = "Toggle";

    // Want 4x simple channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Speed" - a min/max entry between 500-4000
    struct menu_entry menu_speed = new_menu_entry();
    menu_speed.id = menu_ids::SPEED;
    menu_speed.title = "Speed";
    menu_speed.menu_type = menu_entry_type::MIN_MAX;
    menu_speed.minmax.UoM = "mHz"; // Millihertz, not Megahertz (MHz)
    menu_speed.minmax.increment_step = 50; // 3500 (max-min) steps is a bit much, increment by 50 each time to give 70 steps
    menu_speed.minmax.min = 500;
    menu_speed.minmax.max = 4000;
    menu_speed.minmax.current_value = 2000; // start somewhere near the middle
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

void CToggle::get_config(struct routine_conf *conf)
{
   CToggle::config(conf);
}

void CToggle::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    if (menu_id == menu_ids::SPEED)
    {
        _speed_setting = new_value;
    }
}

void CToggle::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    
    // If no change exit
    if 
    (
        (choice_id == 1 &&  _pulse_mode) ||
        (choice_id == 0 && !_pulse_mode)
    )
    {
        return;
    }

    if (menu_id == menu_ids::PULSE_CONT)
    {
        // Switch off before changing mode
        for (int x=0; x < CHANNEL_COUNT; x++)
            simple_channel_off(x);

        _wait_until_us = 0;

        if (choice_id == 1)
            _pulse_mode = true;
        else
            _pulse_mode = false;
    }
}

void CToggle::start()
{
    _current_active_channel = 1;
    _wait_until_us = 0;
}

void CToggle::loop(uint64_t time_us)
{
    if (time_us > _wait_until_us)
    {
        for (int x=0; x < CHANNEL_COUNT; x++)
            simple_channel_set_power(x, POWER_FULL);

        if (_current_active_channel == 1)
        {
            _current_active_channel = 2;
            if (_pulse_mode)
            {
                simple_channel_pulse(1, 100);
                simple_channel_pulse(3, 100);
            }
            else
            {
                simple_channel_off(0);
                simple_channel_off(2);

                simple_channel_on(1);
                simple_channel_on(3);
            }
            
        }
        else
        {
            _current_active_channel = 1;
            if (_pulse_mode)
            {
                simple_channel_pulse(0, 100);
                simple_channel_pulse(2, 100);
            }
            else
            {
                simple_channel_off(1);
                simple_channel_off(3);

                simple_channel_on(0);
                simple_channel_on(2);
            }
        }

        uint64_t wait_for = mHz_to_us_delay(_speed_setting);
        _wait_until_us = time_us + wait_for;
    }
}

void CToggle::stop()
{
   
    for (int x=0; x < CHANNEL_COUNT; x++)    
        simple_channel_off(x);
}

uint64_t CToggle::mHz_to_us_delay(int16_t mHz)
{
    return (uint64_t)1000000000/mHz;
}
