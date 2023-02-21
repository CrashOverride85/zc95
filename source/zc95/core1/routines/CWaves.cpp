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

#include "CWaves.h"
#include "../config.h"
#include <string.h>

static const int pulse_gap_min_us = 2700;
static const int pulse_gap_max_us = (32 * 1000);
static const int initial_speed = 200;

enum menu_ids
{
    WAVES_SPEED = 1
};

CWaves::CWaves(uint8_t param)
{
    printf("CWaves()\n");
}

CWaves::~CWaves()
{
    printf("~CWaves()\n");
}

void CWaves::get_config(struct routine_conf *conf)
{
    CWaves::config(conf);
}

void CWaves::config(struct routine_conf *conf)
{
    conf->name = "Waves";

    // Want four output channels
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    // menu entry 1: "Speed"
    struct menu_entry speed;
    speed.id = menu_ids::WAVES_SPEED;
    speed.title = "Speed";
    speed.menu_type = menu_entry_type::MIN_MAX;
    speed.minmax.UoM = "";
    speed.minmax.increment_step = 10;
    speed.minmax.min = 10;
    speed.minmax.max = 1000;
    speed.minmax.current_value = initial_speed;
    conf->menu.push_back(speed);
}

void CWaves::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    switch (menu_id)
    {
        case menu_ids::WAVES_SPEED:
            _chan[0].pulse_gap_adj_us = new_value;
            _chan[1].pulse_gap_adj_us = new_value * 1.5;
            _chan[2].pulse_gap_adj_us = new_value * 2;
            _chan[3].pulse_gap_adj_us = new_value * 2.5;
            break;
    }
}

void CWaves::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    switch (menu_id)
    {

    }
}

void CWaves::start()
{   
    // Match the power set on front panel, which unless the dials are set to max, isn't the full power the box can ma
    set_all_channels_power(POWER_FULL);

    for (int chan=0; chan < 4; chan++)
    {
        _chan[chan].chan_id = chan;
        _chan[chan].gap_increasing = true;
        _chan[chan].next_pulse_time = 0;
        _chan[chan].pulse_gap_us = (32 * 1000);
    }

    menu_min_max_change(menu_ids::WAVES_SPEED, initial_speed);
}

void CWaves::loop(uint64_t time_us)
{
    for (int chan=0; chan < 4; chan++)
    {
        channel_loop(time_us, &_chan[chan]);
    }
}

void CWaves::channel_loop(uint64_t time_us, chan_info *chan)
{
    if (time_us > chan->next_pulse_time)
    {
        full_channel_pulse(chan->chan_id, DEFAULT_PULSE_WIDTH, DEFAULT_PULSE_WIDTH);

        if (chan->gap_increasing)
        {
            chan->pulse_gap_us += chan->pulse_gap_adj_us;

            if (chan->pulse_gap_us > pulse_gap_max_us)
            {
                chan->pulse_gap_us = pulse_gap_max_us;
                chan->gap_increasing = false;
            } 
        }
        else
        {
           chan->pulse_gap_us -= chan->pulse_gap_adj_us;

            if (chan->pulse_gap_us < pulse_gap_min_us)
            {
                chan->pulse_gap_us = pulse_gap_min_us;
                chan->gap_increasing = true;
            }
        }

        chan->next_pulse_time = time_us + chan->pulse_gap_us;
    }
}

void CWaves::stop()
{
 
}
