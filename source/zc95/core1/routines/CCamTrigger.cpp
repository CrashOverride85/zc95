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

#include "CCamTrigger.h"

enum menu_ids
{
    PULSE_LENGTH = 1,
    CAM_DELAY = 2
};

#define CHANNEL_COUNT 4
static const int DefaultPulseLengthMs = 500;
static const int DefaultCamDelayuMs = 500;

static const int DefaultCamPulseMs = 100;

static const int PulseDelayMs = 300; // This delay corresponds to the response time of the camera + wireless trigger

CCamTrigger::CCamTrigger(uint8_t param)
{
    printf("CCamTrigger()\n");
    _pulse_len_ms = DefaultPulseLengthMs;
    _cam_delay_ms = DefaultCamDelayuMs;
    _cam_pulse_end_us = 0;
    _cam_pulse_start_us = 0;
}

CCamTrigger::~CCamTrigger()
{
    printf("~CCamTrigger()\n");
}

void CCamTrigger::config(struct routine_conf *conf)
{
    conf->name = "Camera Trigger";
    conf->button_text[(int)soft_button::BUTTON_A] = "Trigger";

    // Want 4x simple channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Shock Length"
    struct menu_entry menu_pulse_len = new_menu_entry();
    menu_pulse_len.id = menu_ids::PULSE_LENGTH;
    menu_pulse_len.title = "Shock Length";
    menu_pulse_len.menu_type = menu_entry_type::MIN_MAX;
    menu_pulse_len.minmax.UoM = "ms";
    menu_pulse_len.minmax.increment_step = 50;
    menu_pulse_len.minmax.min = 100;
    menu_pulse_len.minmax.max = 4000;
    menu_pulse_len.minmax.current_value = DefaultPulseLengthMs;
    conf->menu.push_back(menu_pulse_len);

    // menu entry 2: "Camera delay"
    struct menu_entry cam_delay = new_menu_entry();
    cam_delay.id = menu_ids::CAM_DELAY;
    cam_delay.title = "Camera delay";
    cam_delay.menu_type = menu_entry_type::MIN_MAX;
    cam_delay.minmax.UoM = "ms";
    cam_delay.minmax.increment_step = 25;
    cam_delay.minmax.min = 0;
    cam_delay.minmax.max = 2000;
    cam_delay.minmax.current_value = DefaultCamDelayuMs;
    conf->menu.push_back(cam_delay);
}

void CCamTrigger::get_config(struct routine_conf *conf)
{
   CCamTrigger::config(conf);
}

void CCamTrigger::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    if (menu_id == menu_ids::PULSE_LENGTH)
    {
        _pulse_len_ms = new_value;
    }

    if (menu_id == menu_ids::CAM_DELAY)
    {
        _cam_delay_ms = new_value;
    }
}

void CCamTrigger::soft_button_pushed (soft_button button, bool pushed)
{
    if (button == soft_button::BUTTON_A)
    {
        if (pushed)
        {
            _shock_pulse_start_us = time_us_64() + (PulseDelayMs * 1000);
            _cam_pulse_start_us = time_us_64() + (_cam_delay_ms * 1000);
        }
    }
}

void CCamTrigger::start()
{
    set_all_channels_power(POWER_FULL);
}

void CCamTrigger::loop(uint64_t time_us)
{

    if (_shock_pulse_start_us && time_us > _shock_pulse_start_us)
    {
        all_channels_pulse(_pulse_len_ms);
        _shock_pulse_start_us = 0;
    }

    if (_cam_pulse_start_us && time_us > _cam_pulse_start_us)
    {
        _cam_pulse_start_us = 0;
        _cam_pulse_end_us = time_us + (DefaultCamPulseMs * 1000);
        acc_port.set_io_port_state(ExtInputPort::ACC_IO_1, 0);
    }

    if (_cam_pulse_end_us && (time_us > _cam_pulse_end_us))
    {
        _cam_pulse_end_us = 0;
        acc_port.set_io_port_state(ExtInputPort::ACC_IO_1, 1);
    }
}

void CCamTrigger::stop()
{
   set_all_channels_power(0);
    for (int x=0; x < CHANNEL_COUNT; x++)    
        simple_channel_off(x);
}

void CCamTrigger::all_channels(bool on)
{
    for (int x=0; x < CHANNEL_COUNT; x++)   
    {
        if (on)
            simple_channel_on(x);
        else
            simple_channel_off(x);
    }
}

void CCamTrigger::all_channels_pulse(uint16_t pulse_len_ms)
{
    for (int x=0; x < CHANNEL_COUNT; x++)   
    {
        simple_channel_pulse(x, pulse_len_ms);
    }
}
