/*
 * ZC95
 * Copyright (C) 2026  CrashOverride85
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

#include "CMenuRemoteAccessBleSerial.h"

// The BT remote isn't supported here, as connecting to it would reset the BLE stack
CMenuRemoteAccessBleSerial::CMenuRemoteAccessBleSerial(
    CDisplay* display,
    IHal *hal,
    CSavedSettings *saved_settings,
    CRoutineOutput *routine_output,
    std::vector<CRoutines::Routine> &routines,
    CRadio *radio,
    CSdCard* sd_card)
{
    printf("CMenuRemoteAccessBleSerial() \n");
    _display = display;
    _hal = hal;
    _saved_settings = saved_settings;
    _disp_area = _display->get_display_area();
    _exit_menu = false;
    _routine_output = routine_output;
    _radio = radio;

    _ble_serial_connection = new CBleSerialConnection(routine_output, routines, sd_card);
    _routine_output->enable_remote_power_mode();
}

CMenuRemoteAccessBleSerial::~CMenuRemoteAccessBleSerial()
{
    printf("~CMenuRemoteAccessBleSerial()\n");

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_ble_serial_connection != NULL)
    {
        delete _ble_serial_connection;
        _ble_serial_connection = NULL;
    }

    _routine_output->disable_remote_power_mode();
    _radio->bluetooth(false);
}

void CMenuRemoteAccessBleSerial::button_pressed(Button button)
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

void CMenuRemoteAccessBleSerial::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

void CMenuRemoteAccessBleSerial::draw()
{
    int y = ((_disp_area.y1-_disp_area.y0)/2) - 20;
    _display->put_text("BLE serial access", _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    y += 10;
    y += 10;

    if (_ble_serial_connection->is_connected())
    {
        _display->put_text("Connected:", _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        y += 10;
        _display->put_text(std::string(_ble_serial_connection->get_connected_device_address()), _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
    }
    else
    {
        _display->put_text("Advertising as:", _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        y += 10;
        _display->put_text("ZC95", _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
    }

    _ble_serial_connection->set_battery_percentage(_hal->power_management()->get_battery_percentage());
    _ble_serial_connection->loop();
}

void CMenuRemoteAccessBleSerial::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("");
    _display->set_option_d("");

    _exit_menu = false;

    _radio->bluetooth(true);
    _ble_serial_connection->init(_hal->power_management()->get_battery_percentage());
}
