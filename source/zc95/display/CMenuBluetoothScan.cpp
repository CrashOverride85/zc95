/*
 * ZC95
 * Copyright (C) 2023  CrashOverride85
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

#include "CMenuBluetoothScan.h"

CMenuBluetoothScan::CMenuBluetoothScan(CDisplay* display, CSavedSettings *saved_settings, CBluetooth *bluetooth)
{
    printf("CMenuBluetoothScan() \n");
    _display = display;
    _bluetooth = bluetooth;
    _exit_menu = false;
    _options_list = new COptionsList(display, display->get_display_area());
}

CMenuBluetoothScan::~CMenuBluetoothScan()
{
    printf("~CMenuBluetoothScan() \n");
    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_options_list)
    {
        delete _options_list;
        _options_list = NULL;
    }

    _bluetooth->set_state(CBluetooth::state_t::OFF);
}

void CMenuBluetoothScan::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::A: // "Select"
                if (_options_list->count() > 0)
                {
                    _bluetooth->pair(_devices[_options_list->get_current_selection()].address);
                }
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C: // "Up"
                _options_list->up();
                break;

            case Button::D: // "Down"
                _options_list->down();
                break;
        }
        _last_selection = _options_list->get_current_selection();
    }
}

void CMenuBluetoothScan::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

 void CMenuBluetoothScan::draw()
 {
   _bluetooth->scan_get_devices_found(_devices);

   _options_list->clear_options();
    for (std::vector<CBluetoothScan::bt_device_t>::iterator it = _devices.begin(); it != _devices.end(); it++)
    {
        _options_list->add_option((*it).name);
    }

    if (_last_selection > 0)
    {
        _options_list->set_selected(_last_selection);
    }

    _options_list->draw();
 }

void CMenuBluetoothScan::show()
{
    _display->set_option_a("Pair");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _bluetooth->set_state(CBluetooth::state_t::SCAN);

    _exit_menu = false;
}
