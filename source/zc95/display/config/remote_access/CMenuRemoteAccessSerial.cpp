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

#include "CMenuRemoteAccessSerial.h"
#include "CMenuApMode.h"

CMenuRemoteAccessSerial::CMenuRemoteAccessSerial(
    CDisplay* display,
    CGetButtonState *buttons,
    CSavedSettings *saved_settings,
    CRoutineOutput *routine_output,
    std::vector<CRoutines::Routine> &routines,
    CBluetooth *bluetooth)
{
    printf("CMenuRemoteAccessSerial() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _disp_area = _display->get_display_area();
    _exit_menu = false;
    _routine_output = routine_output;
    _bluetooth = bluetooth;
    
    _bt_enabled = _saved_settings->get_bluethooth_enabled();

    _serial_connection = new CSerialConnection(AUX_PORT_UART, routine_output, routines);
    if (_bt_enabled)
    {
        queue_init(&_bt_keypress_queue, sizeof(CBluetoothRemote::bt_keypress_queue_entry_t), 5);
        _bluetooth->set_keypress_queue(&_bt_keypress_queue);
    }

    _routine_output->enable_remote_power_mode();
}

CMenuRemoteAccessSerial::~CMenuRemoteAccessSerial()
{
    printf("~CMenuRemoteAccessSerial()\n");

    if (_bt_enabled)
    {
        _bluetooth->set_keypress_queue(NULL);
        queue_free(&_bt_keypress_queue);
        _bluetooth->set_state(CBluetooth::state_t::OFF);
    }

    if (_serial_connection != NULL)
    {
        delete _serial_connection;
        _serial_connection = NULL;
    }

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    _routine_output->disable_remote_power_mode();
}

void CMenuRemoteAccessSerial::button_pressed(Button button)
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
        }
    }
}

void CMenuRemoteAccessSerial::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

void CMenuRemoteAccessSerial::draw()
{
    int y = ((_disp_area.y1-_disp_area.y0)/2) - 20;
    _display->put_text("Serial control mode"  , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    y += 10;

    _serial_connection->loop();

    // Process input from bluetooth remote, if enabled
    if (_bt_enabled)
    {
        CBluetoothRemote::bt_keypress_queue_entry_t queue_entry;
        while (queue_try_remove(&_bt_keypress_queue, &queue_entry))
        {
            _routine_output->bluetooth_remote_passthrough(queue_entry.key);
        }
    }
}

void CMenuRemoteAccessSerial::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("");
    _display->set_option_d("");

    _exit_menu = false;

    if (_bt_enabled)
    {
        bd_addr_t paired_addr = {0};
        CSavedSettings::bt_device_type_t bt_type = _saved_settings->get_paired_bt_type();
        _saved_settings->get_paired_bt_address(&paired_addr);
        _bluetooth->connect(paired_addr, bt_type);
    }
}
