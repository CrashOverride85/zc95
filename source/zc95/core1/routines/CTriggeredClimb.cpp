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

#include "CTriggeredClimb.h"
#include <string.h>

enum menu_ids
{
    CLIMB_DURATION = 1,
    SHOCK_INC      = 2,
    SHOCK_DURATION = 3
};
 
static const int default_shock_inc_pp = 1;
static const int default_shock_dur_ms = 200;
static const int default_duration_sec = 60; 

CTriggeredClimb::CTriggeredClimb()
{
    printf("CTriggeredClimb()\n");

    _shock_inc_by = default_shock_inc_pp * 10;
    _shock_duration_ms = default_shock_dur_ms;
    
    
    _climbing_power_level = 0;
    _shock_power_level = 1;
    _power_increment_period_ms = 100;
    _next_increment_power_at_us = 0;
    _shock_end_us = 0;
    _shock_required = false;

    _speed_setting = 500;
}

CTriggeredClimb::~CTriggeredClimb()
{
    printf("~CTriggeredClimb()\n");
}

void CTriggeredClimb::config(struct routine_conf *conf)
{
    conf->name = "Triggered Climb";
    conf->button_text[(int)soft_button::BUTTON_A] = "Reset";

    // Lets use all four channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Climb duration" - how long (in seconds) it takes to reach maximum power
    struct menu_entry duration;
    duration.id = menu_ids::CLIMB_DURATION;
    duration.title = "Climb duration";
    duration.menu_type = menu_entry_type::MIN_MAX;
    duration.minmax.UoM = "sec";
    duration.minmax.increment_step = 5;
    duration.minmax.min = 5;
    duration.minmax.max = 120;
    duration.minmax.current_value = default_duration_sec;
    conf->menu.push_back(duration);

    // menu entry 2: "Shock increment" - how much to increment the shock by each time triggered
    struct menu_entry menu_shock_inc;
    menu_shock_inc.id = menu_ids::SHOCK_INC;
    menu_shock_inc.title = "Shock increment by";
    menu_shock_inc.menu_type = menu_entry_type::MIN_MAX;
    menu_shock_inc.minmax.UoM = "p.p.";  // Percentage points
    menu_shock_inc.minmax.increment_step = 1;
    menu_shock_inc.minmax.min = 0;
    menu_shock_inc.minmax.max = 20;
    menu_shock_inc.minmax.current_value = default_shock_inc_pp;
    conf->menu.push_back(menu_shock_inc);

    // menu entry 3: "Shock duration" - how long the shock should last in ms
    struct menu_entry menu_shock_dur;
    menu_shock_dur.id = menu_ids::SHOCK_DURATION;
    menu_shock_dur.title = "Shock duration";
    menu_shock_dur.menu_type = menu_entry_type::MIN_MAX;
    menu_shock_dur.minmax.UoM = "ms"; 
    menu_shock_dur.minmax.increment_step = 50;
    menu_shock_dur.minmax.min = 100;
    menu_shock_dur.minmax.max = 2000;
    menu_shock_dur.minmax.current_value = default_shock_dur_ms;
    conf->menu.push_back(menu_shock_dur);
}

void CTriggeredClimb::get_config(struct routine_conf *conf)
{
   CTriggeredClimb::config(conf);
}

void CTriggeredClimb::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    if (menu_id == menu_ids::SHOCK_INC)
    {
        _shock_inc_by = (new_value * 10);
    }
    else if (menu_id == menu_ids::CLIMB_DURATION)
    {
        // new_value is required total duration in seconds, but as there are 1000 power levels and
        // 1000ms in a second, the maths works out so simple it looks wrong
         _power_increment_period_ms = new_value;
    }
    else if (menu_id == menu_ids::SHOCK_DURATION)
    {
        _shock_duration_ms = new_value;
    }
}

void CTriggeredClimb::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{

}

void CTriggeredClimb::trigger(trigger_socket socket, trigger_part part, bool active)
{
    if (active)
    {
        _shock_required = true;
    }
}

void CTriggeredClimb::soft_button_pushed (soft_button button, bool pushed)
{
    if (pushed && button == soft_button::BUTTON_A)
    {
        reset();
    }
}

void CTriggeredClimb::start()
{
    _climbing_power_level = 0;
    _shock_power_level = 1;
    
    simple_channel_set_power(0, _climbing_power_level);
    simple_channel_set_power(1, _climbing_power_level);

    simple_channel_on(0);
    simple_channel_on(1);

    _shock_required = false;
}

void CTriggeredClimb::loop(uint64_t time_us)
{
    if (_shock_required)
    {
        _shock_end_us = time_us + (1000 * _shock_duration_ms);
        _shock_required = false;
        
        // Climb channels off, and reset power to 0
        simple_channel_off(0);
        simple_channel_off(1);
        _climbing_power_level= 0;

        simple_channel_set_power(0, _climbing_power_level);
        simple_channel_set_power(1, _climbing_power_level);

        // Shock channels on
        simple_channel_on(2);
        simple_channel_on(3);
    }

    else if (_shock_end_us && time_us > _shock_end_us)
    {
        // Shock channels off
        _shock_end_us = 0;
        simple_channel_off(2);
        simple_channel_off(3);

        // Now increment the shock for next time
        _shock_power_level += _shock_inc_by;
        if (_shock_power_level > 1000)
            _shock_power_level = 1000;
        simple_channel_set_power(2, _shock_power_level);
        simple_channel_set_power(3, _shock_power_level);


        // Climb channels on
        _climbing_power_level = 0;
        _next_increment_power_at_us = time_us + (_power_increment_period_ms * 1000);
        simple_channel_on(0);
        simple_channel_on(1);
    }

    else if ((!_shock_end_us && time_us > _next_increment_power_at_us) || _next_increment_power_at_us == 0)
    {
        if (_climbing_power_level < 1000)
            _climbing_power_level++; 

        simple_channel_set_power(0, _climbing_power_level);
        simple_channel_set_power(1, _climbing_power_level);

        _next_increment_power_at_us = time_us + (_power_increment_period_ms * 1000);
    }
}


void CTriggeredClimb::reset() 
{
    simple_channel_off(2);
    simple_channel_off(3);

    _climbing_power_level = 0;
    _shock_power_level = 1;
    _shock_end_us = 0;
    _next_increment_power_at_us = 0;
    
    simple_channel_set_power(0, _climbing_power_level);
    simple_channel_set_power(1, _climbing_power_level);
    simple_channel_set_power(2, _shock_power_level);
    simple_channel_set_power(3, _shock_power_level);

    simple_channel_on(0);
    simple_channel_on(1);
    _shock_required = false;
}

void CTriggeredClimb::stop()
{   
    for (int x = 0; x < 4; x++)
        simple_channel_off(x);
}
