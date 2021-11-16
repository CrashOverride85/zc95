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

#include "CTens.h"
#include <string.h>

enum menu_ids
{
    PULSE_WIDTH = 1,
    FREQUENCY   = 2
};

CTens::CTens()
{
    printf("CTens()\n");
    _pulse_width_us = 130;
    _freq_hz = InitalFrequency;
    _next_pulse_time = 0;
}

CTens::~CTens()
{
    printf("~CTens()\n");
}

void CTens::get_config(struct routine_conf *conf)
{
    CTens::config(conf);
}

void CTens::config(struct routine_conf *conf)
{
    conf->name = "TENS";

    // Need four output channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    // menu entry 1: "pulse width" - a min/max entry between 30-260
    struct menu_entry pulse_width;
    pulse_width.id = menu_ids::PULSE_WIDTH;
    pulse_width.title = "Pulse width";
    pulse_width.menu_type = menu_entry_type::MIN_MAX;
    pulse_width.minmax.UoM = "us";
    pulse_width.minmax.increment_step = 10;
    pulse_width.minmax.min = 30;
    pulse_width.minmax.max = 250;
    pulse_width.minmax.current_value = 130;
    conf->menu.push_back(pulse_width);

    // menu entry 2: "frequency"
    struct menu_entry menu_frequency;
    menu_frequency.id = menu_ids::FREQUENCY;
    menu_frequency.title = "Frequency";
    menu_frequency.menu_type = menu_entry_type::MIN_MAX;
    menu_frequency.minmax.UoM = "Hz";
    menu_frequency.minmax.increment_step = 20;
    menu_frequency.minmax.min = 1;
    menu_frequency.minmax.max = 2000;
    menu_frequency.minmax.current_value = InitalFrequency;
    conf->menu.push_back(menu_frequency);
}

void CTens::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    switch (menu_id)
    {
        case menu_ids::FREQUENCY:
            _freq_hz = new_value;
            break;

        case menu_ids::PULSE_WIDTH:
            _pulse_width_us = new_value;
            break;
    }
}

void CTens::start()
{
    set_all_channels_power(POWER_FULL);
    _next_pulse_time = 0;
}

void CTens::loop(uint64_t time_us)
{
    if (time_us > _next_pulse_time)
    {
        for (int x=0; x < 4; x++)
        {
            full_channel_pulse(x, _pulse_width_us, _pulse_width_us);
        }

        _next_pulse_time = time_us + hz_to_us_delay(_freq_hz);
    }
}

void CTens::stop()
{
 
}

uint64_t CTens::hz_to_us_delay(int16_t hz)
{
    return (uint64_t)1000000/hz;
}
