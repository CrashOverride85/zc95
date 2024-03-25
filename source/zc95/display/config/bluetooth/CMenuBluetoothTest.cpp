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

    queue_init(&_bt_keypress_queue, sizeof(CBluetoothRemote::bt_keypress_queue_entry_t), 5);
    queue_init(&_bt_raw_hid_queue , sizeof(CBluetoothConnect::bt_raw_hid_queue_entry_t), 5);

    _bluetooth->set_keypress_queue(&_bt_keypress_queue);
    _bluetooth->set_bt_raw_hid_queue(&_bt_raw_hid_queue);
}

CMenuBluetoothTest::~CMenuBluetoothTest()
{
    printf("~CMenuBluetoothTest()\n");
    _bluetooth->set_keypress_queue(NULL);
    _bluetooth->set_bt_raw_hid_queue(NULL);
    queue_free(&_bt_keypress_queue);
    queue_free(&_bt_raw_hid_queue);

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
    _display->put_text("State: ", _disp_area.x0, _disp_area.y0, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    CBluetoothConnect::bt_connect_state_t connect_state = _bluetooth->get_connect_state();
    _display->put_text(CBluetoothConnect::s_state_to_string(connect_state), _disp_area.x0, _disp_area.y0+8, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));

    if (_bt_device_type == CSavedSettings::bt_device_type_t::HID)
        draw_bt_remote();
    else
        draw_bt_other();
}

void CMenuBluetoothTest::draw_bt_remote()
{
    CBluetoothRemote::bt_keypress_queue_entry_t queue_entry;
    while (queue_try_remove(&_bt_keypress_queue, &queue_entry))
    {
        _message = CBluetoothRemote::s_get_keypress_string(queue_entry.key);
        _keypress_displayed_us = time_us_64();
    }

    // If the keypress was received in the last 100ms, make the text yellow, otherwise stick to white
    hagl_color_t text_colour; 
    if (time_us_64() - _keypress_displayed_us < 1000 * 100)
        text_colour = hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0x00);
    else
        text_colour = hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70);

    _display->put_text("Key:"  , _disp_area.x0, _disp_area.y0 + ((_disp_area.y1-_disp_area.y0)/2)-10 , hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    _display->put_text(_message, _disp_area.x0, _disp_area.y0 + ((_disp_area.y1-_disp_area.y0)/2)    , text_colour);
}

void CMenuBluetoothTest::draw_bt_other()
{
    CBluetoothConnect::bt_raw_hid_queue_entry_t queue_entry;
    if (queue_try_remove(&_bt_raw_hid_queue, &queue_entry))
    {
        char msg_buffer[100] = {0};
        snprintf(msg_buffer, sizeof(msg_buffer), 
            "Usage page = 0x%X\n"
            "Usage      = 0x%X\n"
            "Value      = %lu\n",
            queue_entry.usage_page,
            queue_entry.usage,
            queue_entry.value);
        
        _message = msg_buffer;
    }

    _display->put_text(_message, _disp_area.x0, _disp_area.y0+30, hagl_color(_display->get_hagl_backed(), 0x70, 0x70, 0x70));
}

void CMenuBluetoothTest::show()
{
    _display->set_option_b("Back");
    _display->set_option_a("");
    _display->set_option_c("");
    _display->set_option_d("");

    bd_addr_t paired_addr = {0};
    _bt_device_type = g_SavedSettings->get_paired_bt_type();
    g_SavedSettings->get_paired_bt_address(&paired_addr);

    _bluetooth->connect(paired_addr, _bt_device_type);

    _exit_menu = false;
}
