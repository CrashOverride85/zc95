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

#include "CRoutineOutputDebug.h"



CRoutineOutputDebug::CRoutineOutputDebug(Core1 *core1, CDisplay *display)
{
    _core1 = core1;
    _display = display;
}

void CRoutineOutputDebug::set_front_panel_power(uint8_t channel, uint16_t power)
{
    _core1->power_level_control->set_front_panel_power(channel, power);
    update_display(channel);
    _core1->update_channel_power(channel);
   
}

uint16_t CRoutineOutputDebug::get_output_power(uint8_t channel)
{
    return _core1->power_level_control->get_output_power_level(channel);
}

uint16_t CRoutineOutputDebug::get_front_pannel_power(uint8_t channel)
{
    return _core1->power_level_control->get_target_max_power_level(channel);
}

void CRoutineOutputDebug::activate_routine(uint8_t routine_id)
{
    _core1->activate_routine(routine_id);
}

void CRoutineOutputDebug::stop_routine()
{
    _core1->stop_routine();
}

void CRoutineOutputDebug::update_display(uint8_t channel)
{
    _display->set_power_level(channel, get_front_pannel_power(channel), get_output_power(channel), _core1->power_level_control->get_max_power_level(channel));
}

void CRoutineOutputDebug::menu_min_max_change(uint8_t menu_id, int16_t new_value)
{
    _core1->menu_min_max_change(menu_id, new_value);
}

void CRoutineOutputDebug::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    _core1->menu_multi_choice_change(menu_id, choice_id);
}

void CRoutineOutputDebug::soft_button_pressed(soft_button button)
{
    _core1->soft_button_pushed(button, true);
}

void CRoutineOutputDebug::trigger(trigger_socket socket, trigger_part part, bool active)
{
    _core1->trigger(socket, part, active);
}

void CRoutineOutputDebug::loop()
{
    _core1->loop();
}

void CRoutineOutputDebug::collar_transmit (uint16_t id, CCollarComms::collar_channel channel, CCollarComms::collar_mode mode, uint8_t power)
{
    _core1->collar_transmit(id, channel, mode, power);
}

void CRoutineOutputDebug::reinit_channels()
{
    _core1->stop_routine();
    _core1->init();
}