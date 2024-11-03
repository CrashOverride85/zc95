/*
 * ZC95
 * Copyright (C) 2024  CrashOverride85
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

#include "CMenuRemoteAccessBLE.h"
#include "CMenuApMode.h"

CMenuRemoteAccessBLE::CMenuRemoteAccessBLE(
    CDisplay* display,
    CGetButtonState *buttons,
    CSavedSettings *saved_settings,
    CRoutineOutput *routine_output,
    std::vector<CRoutines::Routine> &routines,
    CRadio *radio)
{
    printf("CMenuRemoteAccessBLE() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _disp_area = _display->get_display_area();
    _exit_menu = false;
    _routine_output = routine_output;
    _radio = radio;
    
    if (_saved_settings->get_ble_remote_access_power_dial_mode() == CSavedSettings::ble_power_dial_mode_t::LIMIT)
        _routine_output->enable_remote_power_mode();

    _gatt_server = new CBtGatt(routine_output, routines, _saved_settings);
}

CMenuRemoteAccessBLE::~CMenuRemoteAccessBLE()
{
    printf("~CMenuRemoteAccessBLE()\n");

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_gatt_server != NULL)
    {
        delete _gatt_server;
        _gatt_server = NULL;
    }

    if (_saved_settings->get_ble_remote_access_power_dial_mode() == CSavedSettings::ble_power_dial_mode_t::LIMIT)
        _routine_output->disable_remote_power_mode();

     _radio->bluetooth(false);
}

void CMenuRemoteAccessBLE::button_pressed(Button button)
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
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C: // "Up"
                break;

            case Button::D: // "Down"
                break;

            case Button::ROT:
                break;
        }
    }
}

void CMenuRemoteAccessBLE::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

void CMenuRemoteAccessBLE::draw()
{
    int y = 0;
    _display->put_text("BLE Remote access"  , _disp_area.x0, _disp_area.y0 + y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    y += 10;
    y += 10;

    if (_gatt_server->is_connected())
    {
        _display->put_text("Connected:"  , _disp_area.x0, _disp_area.y0 + y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        y += 10;
        _display->put_text(std::string(_gatt_server->get_connected_device_address())  , _disp_area.x0, _disp_area.y0 + y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
        y += 10;
    }
    else
    {
        y += 10;
        y += 10;
    }
    y += 10;

    _display->put_text("Triphase:"  , _disp_area.x0, _disp_area.y0 + y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    y += 10;

    if (!gZc624ChannelIsolationEnabled)
    {
        hagl_color_t red = hagl_color(_display->get_hagl_backed(), 0xFF, 0x00, 0x00);
        _display->put_text("  ACTIVE", _disp_area.x0, _disp_area.y0 + y, red);        
    }

    else if (_saved_settings->get_ble_remote_disable_channel_isolation_permitted())
    {
        hagl_color_t yellow = hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0x00);
        _display->put_text("  Permitted", _disp_area.x0, _disp_area.y0 + y, yellow);
    }
    else
    {
        hagl_color_t grey = hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70);
        _display->put_text("  Disabled", _disp_area.x0, _disp_area.y0 + y, grey);
    }
    y += 10;

    
    _gatt_server->set_battery_percentage(_display->get_battery_percentage());
    _gatt_server->loop();
}

void CMenuRemoteAccessBLE::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("");
    _display->set_option_d("");

    _exit_menu = false;

    _radio->bluetooth(true);
    _gatt_server->init(_display->get_battery_percentage());
}
