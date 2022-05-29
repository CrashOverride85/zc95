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
    pulse_width.minmax.min = 10;
    pulse_width.minmax.max = 250;
    pulse_width.minmax.current_value = DEFAULT_PULSE_WIDTH;
    conf->menu.push_back(pulse_width);

    // menu entry 2: "frequency"
    struct menu_entry menu_frequency;
    menu_frequency.id = menu_ids::FREQUENCY;
    menu_frequency.title = "Frequency";
    menu_frequency.menu_type = menu_entry_type::MIN_MAX;
    menu_frequency.minmax.UoM = "Hz";
    menu_frequency.minmax.increment_step = 5;
    menu_frequency.minmax.min = 5;
    menu_frequency.minmax.max = 250;
    menu_frequency.minmax.current_value = DEFAULT_FREQ_HZ;
    conf->menu.push_back(menu_frequency);
}

void CTens::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    switch (menu_id)
    {
        case menu_ids::FREQUENCY:
            set_freq_all(new_value);
            break;

        case menu_ids::PULSE_WIDTH:
            set_pulse_width_all(new_value);
            break;
    }
}

void CTens::start()
{
    set_pulse_width_all(DEFAULT_PULSE_WIDTH);
    set_freq_all(DEFAULT_FREQ_HZ);
    set_all_channels_power(POWER_FULL);
    full_channel_on(0);
    full_channel_on(1);
    full_channel_on(2);
    full_channel_on(3);
}

void CTens::loop(uint64_t time_us)
{

}

void CTens::stop()
{
 
}

void CTens::set_pulse_width_all(uint8_t pulse_width)
{
    for (uint8_t chan = 0; chan < 4; chan++)
        full_channel_set_pulse_width(chan, pulse_width, pulse_width);
}

void CTens::set_freq_all(uint16_t freq)
{
    for (uint8_t chan = 0; chan < 4; chan++)
        full_channel_set_freq(chan, freq);
}
