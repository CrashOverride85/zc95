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

#include "CMenuBluetoothTest.h"
#include "../globals.h"

CMenuBluetoothTest::CMenuBluetoothTest(CDisplay* display, CBluetooth *bluetooth)
{
    printf("CMenuBluetoothTest()\n");
    _display = display;
    _bluetooth = bluetooth;
    _exit_menu = false;
    _disp_area = _display->get_display_area();
}

CMenuBluetoothTest::~CMenuBluetoothTest()
{
    printf("~CMenuBluetoothTest()\n");
    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    _bluetooth->set_state(CBluetooth::state_t::OFF);
}

void CMenuBluetoothTest::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::A:
                return;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C:
                break;

            case Button::D:
                break;
        }
    }
}

void CMenuBluetoothTest::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

 void CMenuBluetoothTest::draw()
 {

    /*
 
    {
        std::string message;
        switch (_bluetooth->get_pair_state())
        {
            case CBluetoothPair::bt_pair_state_t::SUCCESS:
                message = "Success!";
                break;

            case CBluetoothPair::bt_pair_state_t::START:
                message = "In progress...";
                break;

            case CBluetoothPair::bt_pair_state_t::FAILED:
                message = "Failed!";
                break;
            break;

            default: // Unknown & IDLE
                message = "ERROR";
                break;
        }

        int y = ((_disp_area.y1-_disp_area.y0)/2) - 30;
        _display->put_text("Paring status:" , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        y += 10;
        _display->put_text(message          , _disp_area.x0, _disp_area.y0+y, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
        y += 10;
    }
    */
}

void CMenuBluetoothTest::show()
{
    _display->set_option_b("Back");
    _display->set_option_a("");
    _display->set_option_c("");
    _display->set_option_d("");

    bd_addr_t paired_addr = {0};
    g_SavedSettings->get_paired_bt_address(&paired_addr);

    _bluetooth->connect(paired_addr);

    _exit_menu = false;
}

