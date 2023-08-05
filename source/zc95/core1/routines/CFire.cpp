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

#include "CFire.h"

enum menu_ids
{
    MODE = 1,
    PULSE_LENGTH = 2
};

#define CHANNEL_COUNT 4
static const int DefaultPulseLengthMs = 500;

CFire::CFire(uint8_t param)
{
    printf("CFire()\n");
    _pulse_len_ms = DefaultPulseLengthMs;
    _pulse_mode = false;
}

CFire::~CFire()
{
    printf("~CFire()\n");
}

void CFire::config(struct routine_conf *conf)
{
    conf->name = "Fire";
    conf->button_text[(int)soft_button::BUTTON_A] = "Fire";

    // Want 4x simple channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Mode" - pulse for present length, or continuous whilst button pressed
    struct menu_entry menu_mode = new_menu_entry();
    menu_mode.id = menu_ids::MODE;
    menu_mode.title = "Mode";
    menu_mode.menu_type = menu_entry_type::MULTI_CHOICE;
    menu_mode.multichoice.current_selection = 0;
    menu_mode.multichoice.choices.push_back(get_choice("Continuous", 0));
    menu_mode.multichoice.choices.push_back(get_choice("Pulse", 1));
    conf->menu.push_back(menu_mode);

    // menu entry 2: "Pulse"
    struct menu_entry menu_pulse_len = new_menu_entry();
    menu_pulse_len.id = menu_ids::PULSE_LENGTH;
    menu_pulse_len.title = "Pulse length";
    menu_pulse_len.menu_type = menu_entry_type::MIN_MAX;
    menu_pulse_len.minmax.UoM = "ms";
    menu_pulse_len.minmax.increment_step = 50;
    menu_pulse_len.minmax.min = 100;
    menu_pulse_len.minmax.max = 4000;
    menu_pulse_len.minmax.current_value = DefaultPulseLengthMs;
    conf->menu.push_back(menu_pulse_len);
}

void CFire::get_config(struct routine_conf *conf)
{
   CFire::config(conf);
}

void CFire::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    if (menu_id == menu_ids::PULSE_LENGTH)
    {
        _pulse_len_ms = new_value;
    }
}

void CFire::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    
    if (menu_id == menu_ids::MODE)
    {
        if (choice_id == 1)
            _pulse_mode = true;
        else
            _pulse_mode = false;
    }
}

void CFire::soft_button_pushed (soft_button button, bool pushed)
{
    if (button == soft_button::BUTTON_A)
    {
        if (_pulse_mode)
        {
            // Pulse mode (pulse for preset length when button pushed)
            if (pushed)
            {
                all_channels_pulse(_pulse_len_ms);
            }
        }
        else
        {
            // Continous mode (on whilst button held down)
            if (pushed)
                all_channels(true);
            else
                all_channels(false);
        }
    }
}

void CFire::trigger(trigger_socket socket, trigger_part part, bool active)
{
    // External trigger input is the same as pressing the "Fire" soft-button
    soft_button_pushed(soft_button::BUTTON_A, active);
}

void CFire::start()
{
    set_all_channels_power(POWER_FULL);
}

void CFire::loop(uint64_t time_us)
{

}

void CFire::stop()
{
   set_all_channels_power(0);
    for (int x=0; x < CHANNEL_COUNT; x++)    
        simple_channel_off(x);
}

void CFire::all_channels(bool on)
{
    for (int x=0; x < CHANNEL_COUNT; x++)   
    {
        if (on)
            simple_channel_on(x);
        else
            simple_channel_off(x);
    }
}

void CFire::all_channels_pulse(uint16_t pulse_len_ms)
{
    for (int x=0; x < CHANNEL_COUNT; x++)   
    {
        simple_channel_pulse(x, pulse_len_ms);
    }
}
