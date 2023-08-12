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

#include "CBuzz.h"

enum menu_ids
{
    GAME_DURATION = 1,
    SHOCK_INC_PP  = 2,
    INITAL_POWER  = 3,
    SHOCK_LENGTH  = 4
};

static const int DefaultGameLenghtSec    = 50;
static const int DefaultShockIncrementPP = 5;
static const int DefaultShockLengthMs    = 300;
static const int DefaultInitialPowerPc   = 15;

#define CHANNEL_COUNT 4

CBuzz::CBuzz(uint8_t param)
{
    printf("CBuzz()\n");
    _power_increment_period_ms = DefaultGameLenghtSec;
    _shock_increment_pp = DefaultShockIncrementPP;
    _shock_len_ms = DefaultShockLengthMs;
    _inital_shock_power_level = DefaultInitialPowerPc * 10; // power% => 0-1000
    stop_game();
}

CBuzz::~CBuzz()
{
    printf("~CBuzz()\n");
}

void CBuzz::config(struct routine_conf *conf)
{
    conf->name = "Buzz";
    conf->button_text[(int)soft_button::BUTTON_A] = "Start";

    // Want 4x simple channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // Game length
    struct menu_entry menu_game_len = new_menu_entry();
    menu_game_len.id = menu_ids::GAME_DURATION;
    menu_game_len.title = "Game length";
    menu_game_len.menu_type = menu_entry_type::MIN_MAX;
    menu_game_len.minmax.UoM = "sec";
    menu_game_len.minmax.increment_step = 10;
    menu_game_len.minmax.min = 10;
    menu_game_len.minmax.max = 300;
    menu_game_len.minmax.current_value = DefaultGameLenghtSec;
    conf->menu.push_back(menu_game_len);

    // Shock increment, percentage points
    struct menu_entry menu_shock_increment = new_menu_entry();
    menu_shock_increment.id = menu_ids::SHOCK_INC_PP;
    menu_shock_increment.title = "Shock increment";
    menu_shock_increment.menu_type = menu_entry_type::MIN_MAX;
    menu_shock_increment.minmax.UoM = "p.p.";
    menu_shock_increment.minmax.increment_step = 1;
    menu_shock_increment.minmax.min = 0;
    menu_shock_increment.minmax.max = 10;
    menu_shock_increment.minmax.current_value = DefaultShockIncrementPP;
    conf->menu.push_back(menu_shock_increment);

    // Initial power
    struct menu_entry menu_inital_power = new_menu_entry();
    menu_inital_power.id = menu_ids::INITAL_POWER;
    menu_inital_power.title = "Inital shock power";
    menu_inital_power.menu_type = menu_entry_type::MIN_MAX;
    menu_inital_power.minmax.UoM = "%";
    menu_inital_power.minmax.increment_step = 1;
    menu_inital_power.minmax.min = 0;
    menu_inital_power.minmax.max = 100;
    menu_inital_power.minmax.current_value = DefaultInitialPowerPc;
    conf->menu.push_back(menu_inital_power);

    // Shock length
    struct menu_entry menu_shock_length = new_menu_entry();
    menu_shock_length.id = menu_ids::SHOCK_LENGTH;
    menu_shock_length.title = "Min shock length";
    menu_shock_length.menu_type = menu_entry_type::MIN_MAX;
    menu_shock_length.minmax.UoM = "ms";
    menu_shock_length.minmax.increment_step = 100;
    menu_shock_length.minmax.min = 100;
    menu_shock_length.minmax.max = 2000;
    menu_shock_length.minmax.current_value = DefaultShockLengthMs;
    conf->menu.push_back(menu_shock_length);
}

void CBuzz::get_config(struct routine_conf *conf)
{
   CBuzz::config(conf);
}

void CBuzz::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    switch (menu_id)
    {
        case menu_ids::GAME_DURATION:
            _power_increment_period_ms = new_value;
            break;

        case menu_ids::SHOCK_INC_PP:
            _shock_increment_pp = new_value;
            break;

        case menu_ids::INITAL_POWER:
            _inital_shock_power_level = new_value * 10; // new_value is power level in %, so convert to 0-1000 power levels

            if ((_shock_power_level < _inital_shock_power_level) || !_game_running)
            {
                _shock_power_level = _inital_shock_power_level;
                shock_power_level(_shock_power_level);
            }
            break;

        case menu_ids::SHOCK_LENGTH:
            _shock_len_ms = new_value;
            break;
    }
}

void CBuzz::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    

}

void CBuzz::soft_button_pushed (soft_button button, bool pushed)
{
    if (button == soft_button::BUTTON_A)
    {
        if (pushed)
        {
            if (!_game_running)
                start_game();
        }
    }
}

void CBuzz::trigger(trigger_socket socket, trigger_part part, bool active)
{
    if (!_game_running)
        return;

    if (socket != trigger_socket::Trigger1)
        return;

    // Part A = Handle touching end. Make sure the signal is present for at least x milliseconds to avoid false triggering
    if (part == trigger_part::A)
    {
        if (active)
        {
            if (!_end_game_at_us)
            {
                const int delay_ms = 250;
                _end_game_at_us = time_us_64() + (1000 * delay_ms);
            }
        }
        else
        {
            _end_game_at_us = 0;
        }
    }

    // Part B = Handle touching wire
    else if (part == trigger_part::B)
    {
        _shock_trigger_active = active;

        if (active)
        {
            if (_shock_start_time_us == 0)
            {
                shock_start();
            }
        }
    }
}

// This is called then the routine is first started. Calling stop_game() is a 
// good way to init everything ready for start_game() to be called
void CBuzz::start()
{
    stop_game();
}

void CBuzz::loop(uint64_t time_us)
{
    if (!_game_running)
        return;

    if (_end_game_at_us && time_us > _end_game_at_us)
    {
        stop_game();
        return;
    }

    // Deal with the shock on chan 2+3
    if (_shock_start_time_us)
    {
        if (!_shock_trigger_active && (time_us > _shock_start_time_us + (_shock_len_ms * 1000)))
        {
            shock_stop();
        }

        uint32_t shock_increment_us = (1000 * 1000 * 2); // 2 seconds
        if (time_us > _last_shock_increment_us + shock_increment_us)
        {
            _shock_power_level += (_shock_increment_pp * 10);
            if (_shock_power_level > 1000)
                _shock_power_level = 1000;
            shock_power_level(_shock_power_level);
            _last_shock_increment_us = time_us;
        }
    }

    // Increasing level on chan 1+0
    if (time_us > _next_power_increase_us)
    {
        if (_current_power_level < 1000)
            _current_power_level++;

        _next_power_increase_us = time_us + (_power_increment_period_ms * 1000);
        simple_channel_set_power(0, _current_power_level);
        simple_channel_set_power(1, _current_power_level);
    }
}

void CBuzz::stop()
{
   set_all_channels_power(0);
    for (int x=0; x < CHANNEL_COUNT; x++)
        simple_channel_off(x);
}

void CBuzz::shock_start()
{
    simple_channel_on(2);
    simple_channel_on(3);
    _shock_start_time_us = time_us_64();
}

void CBuzz::shock_stop()
{
    simple_channel_off(2);
    simple_channel_off(3);
    _shock_start_time_us = 0;
}

void CBuzz::shock_power_level(uint16_t new_power_level)
{
    simple_channel_set_power(2, new_power_level);
    simple_channel_set_power(3, new_power_level);
}

void CBuzz::stop_game()
{
    _game_running = false;
    _next_power_increase_us = 0;
    _current_power_level = 0;
    _last_shock_increment_us = 0;
    _shock_power_level = _inital_shock_power_level;
    _shock_start_time_us = 0;
    _shock_trigger_active = false;
    _end_game_at_us = 0;

    simple_channel_set_power(0, _current_power_level);
    simple_channel_set_power(1, _current_power_level);
    shock_power_level(_shock_power_level);
    for (int x=0; x < CHANNEL_COUNT; x++)    
        simple_channel_off(x);
}

void CBuzz::start_game()
{
    _next_power_increase_us = 0;
    _current_power_level = 0;
    _last_shock_increment_us = 0;
    _shock_power_level = _inital_shock_power_level;
    _shock_start_time_us = 0;
    _shock_trigger_active = false;
    _end_game_at_us = 0;

    simple_channel_set_power(0, _current_power_level);
    simple_channel_set_power(1, _current_power_level);
    shock_power_level(_shock_power_level);
    simple_channel_on(0);
    simple_channel_on(1);
    _game_running = true;
}

void CBuzz::set_power_increment_ms_from_game_length(uint16_t game_len_sec)
{
    // game_len_sec is required total duration in seconds, and _power_increment_period_ms is how long to
    // stay at each power level for before increasing it. As there are 1000 power levels and 1000ms in a 
    // second, the maths works out so simple it looks wrong
    _power_increment_period_ms = game_len_sec;
}
