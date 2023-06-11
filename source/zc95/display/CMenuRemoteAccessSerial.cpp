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
    CAnalogueCapture *analogue_capture,
    std::vector<CRoutines::Routine> *routines)
{
    printf("CMenuRemoteAccessSerial() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _disp_area = _display->get_display_area();
    _exit_menu = false;
    _routine_output = routine_output;
    _analogue_capture = analogue_capture;

    _serial_connection = new CSerialConnection(AUX_PORT_UART, analogue_capture, routine_output, routines);

    _routine_output->enable_remote_power_mode();
}

CMenuRemoteAccessSerial::~CMenuRemoteAccessSerial()
{
    printf("~CMenuRemoteAccessSerial()\n");

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
        _display->put_text("Serial control mode"  , _disp_area.x0, _disp_area.y0+y, hagl_color(0xFF, 0xFF, 0xFF));
        y += 10;

        _serial_connection->loop();
}

void CMenuRemoteAccessSerial::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("");
    _display->set_option_d("");

    _exit_menu = false;
}
