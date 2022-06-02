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

#include "CShockChoice.h"
#include <string.h>

enum menu_ids
{
    CHOICE_FREQUENCY = 1,
    SHOCK_INC        = 2
};

static const int warn_interval_sec = 3;

static const int default_choice_freq_sec = 10; 
static const int default_shock_inc_pp = 1;

CShockChoice::CShockChoice()
{
    printf("CShockChoice()\n");
    _choice_freq_sec = default_choice_freq_sec;
    _shock_inc_by = default_shock_inc_pp * 10;
    reset();
}

CShockChoice::~CShockChoice()
{
    printf("~CShockChoice()\n");
}

void CShockChoice::config(struct routine_conf *conf)
{
    conf->name = "Shock choice";
    conf->button_text[(int)soft_button::BUTTON_A] = "Reset";

    // Lets use all four channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Choice frequency" - how often a choice needs to be made
    struct menu_entry duration;
    duration.id = menu_ids::CHOICE_FREQUENCY;
    duration.title = "Choice frequency";
    duration.menu_type = menu_entry_type::MIN_MAX;
    duration.minmax.UoM = "sec";
    duration.minmax.increment_step = 2;
    duration.minmax.min = 8;
    duration.minmax.max = 120;
    duration.minmax.current_value = default_choice_freq_sec;
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
}

void CShockChoice::get_config(struct routine_conf *conf)
{
   CShockChoice::config(conf);
}

void CShockChoice::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    if (menu_id == menu_ids::SHOCK_INC)
    {
        _shock_inc_by = (new_value * 10);
    }
    else if (menu_id == menu_ids::CHOICE_FREQUENCY)
    {
        _choice_freq_sec = new_value;
    }
}

void CShockChoice::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{

}

void CShockChoice::trigger(trigger_socket socket, trigger_part part, bool active)
{
    // Not interested in either button being released
    if (!active)
        return;

    if (socket == trigger_socket::Trigger1)
    {
        pulse_chan_and_inc_power(channel::CHAN_A, 500);
        reset_timer();
    }

    if (socket == trigger_socket::Trigger2)
    {
        pulse_chan_and_inc_power(channel::CHAN_B, 500);
        reset_timer();
    }
}

void CShockChoice::soft_button_pushed (soft_button button, bool pushed)
{
    if (pushed && button == soft_button::BUTTON_A)
    {
        reset();
    }
}

void CShockChoice::start()
{
    simple_channel_set_power(3, POWER_FULL);
}

void CShockChoice::loop(uint64_t time_us)
{
    if (_timer_started_us == 0)
    {
        _timer_started_us = time_us;
    }

    if (!_warning_issued && (time_remaining_us() < secs_to_us(warn_interval_sec)))
    {
        simple_channel_pulse(3, 100);
        _warning_issued = true;
    }

    if (time_remaining_us() <= 0)
    {
        pulse_chan_and_inc_power(channel::CHAN_A, 1000);
        pulse_chan_and_inc_power(channel::CHAN_B, 1000);
        reset_timer();
    }
}

int64_t CShockChoice::time_remaining_us()
{
    return secs_to_us(_choice_freq_sec) - ((time_us_64() - _timer_started_us));
}

void CShockChoice::reset_timer()
{
    _timer_started_us = time_us_64();
    _warning_issued = false;
}

void CShockChoice::pulse_chan_and_inc_power(channel chan, uint16_t duration)
{
    if (chan == channel::CHAN_A)
    {
        simple_channel_pulse(0, duration);
        _chanA_power += _shock_inc_by;
        if (_chanA_power > 1000)
            _chanA_power = 1000;

        simple_channel_set_power(0, _chanA_power);
    } 
    
    else if (chan == channel::CHAN_B)
    {
        simple_channel_pulse(1, duration);
        simple_channel_pulse(2, duration);
        _chanB_power += _shock_inc_by;
        if (_chanB_power > 1000)
            _chanB_power = 1000;
        
        simple_channel_set_power(1, _chanB_power);
        simple_channel_set_power(2, _chanB_power);
    }
}

void CShockChoice::reset() 
{
    _chanA_power = 0;
    _chanB_power = 0;
    simple_channel_set_power(0, _chanA_power);
    simple_channel_set_power(1, _chanB_power);
    simple_channel_set_power(2, _chanB_power);

    _timer_started_us = 0;
    _warning_issued = false;
}

void CShockChoice::stop()
{   
    for (int x = 0; x < 4; x++)
        simple_channel_off(x);
}

int64_t inline CShockChoice::secs_to_us(int seconds)
{
    return ((uint64_t)seconds * 1000000);
}
