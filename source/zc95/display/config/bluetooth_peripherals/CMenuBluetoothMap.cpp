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

#include "CMenuBluetoothMap.h"
#include "../CDebugOutput.h"
#include "../config.h"


CMenuBluetoothMap::CMenuBluetoothMap(CDisplay* display, CSavedSettings *saved_settings, CBluetooth *bluetooth)
{
    printf("CMenuBluetoothMap()\n");
    _display = display;
    _saved_settings = saved_settings;
    _bluetooth = bluetooth;

    _exit_menu = false;
    display_area area = display->get_display_area();
    area.y1 = area.y0 + ((area.y1-area.y0)/2);
    _keypress_list = new COptionsList(display, area);

    _setting_choice_area = display->get_display_area();
    _setting_choice_area.y0 = _setting_choice_area.y1 - ((_setting_choice_area.y1-_setting_choice_area.y0)/2);
    _keypress_action_list = new COptionsList(display, _setting_choice_area);
}

CMenuBluetoothMap::~CMenuBluetoothMap()
{
    printf("~CMenuBluetoothMap()\n");

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_keypress_list)
    {
        delete _keypress_list;
        _keypress_list = NULL;
    }

    if (_keypress_action_list)
    {
        delete _keypress_action_list;
        _keypress_action_list = NULL;
    }
}

void CMenuBluetoothMap::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        if (button == Button::B) // "Back"
        {
            _exit_menu = true;
        }

        if (button == Button::C || button == Button::D)
        {
            if (button == Button::C) // "Up"
            {
                _keypress_list->up();
            }

            if (button == Button::D) // "Down"
            {
                _keypress_list->down();
            }

            CBluetoothRemote::keypress_t key  = (CBluetoothRemote::keypress_t)_remote_keypress[_keypress_list->get_current_selection()].id;
            set_actions_on_bottom_list(_saved_settings->get_bt_keypress_action(key));
        }
    }
}

void CMenuBluetoothMap::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change >= 1)
        {
            _keypress_action_list->down();
        }
        else if (change <= -1)
        {
            _keypress_action_list->up();
        }

        CBluetoothRemote::keypress_t key  = (CBluetoothRemote::keypress_t)_remote_keypress[_keypress_list->get_current_selection()].id;
        CBluetoothRemote::keypress_action_t action = (CBluetoothRemote::keypress_action_t)_keypress_action[_keypress_action_list->get_current_selection()].id;
        
        _saved_settings->set_bt_keypress_action(key, action);
    }
}

void CMenuBluetoothMap::draw()
{
    _keypress_list->draw();

    // Show list of options for the selected keypress in blue box at bottom of screen
    hagl_color_t rect_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);
    hagl_fill_rectangle(_display->get_hagl_backed(), _setting_choice_area.x0, _setting_choice_area.y0,
                        _setting_choice_area.x1, _setting_choice_area.y1, rect_colour);
    _keypress_action_list->draw();
}

void CMenuBluetoothMap::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _remote_keypress.clear();
    _remote_keypress.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_SHUTTER, "Shutter"));
    _remote_keypress.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_BUTTON , "Button" ));
    _remote_keypress.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_UP     , "Up"     ));
    _remote_keypress.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_DOWN   , "Down"   ));
    _remote_keypress.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_LEFT   , "Left"   ));
    _remote_keypress.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_RIGHT  , "Right"  ));
    _remote_keypress.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_UNKNOWN, "Unknown"));

    _keypress_list->clear_options();
    for (std::vector<CMenuBluetoothMap::setting_t>::iterator it = _remote_keypress.begin(); it != _remote_keypress.end(); it++)
    {
        _keypress_list->add_option((*it).text);
    }

    _exit_menu = false;
    set_actions_on_bottom_list(_saved_settings->get_bt_keypress_action(CBluetoothRemote::keypress_t::KEY_SHUTTER));
}

void CMenuBluetoothMap::set_actions_on_bottom_list(CBluetoothRemote::keypress_action_t current_action)
{
    _keypress_action.clear();

    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::NONE      , "<none>"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::BUT_A     , "Top left soft"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::BUT_B     , "Bottom left soft"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::BUT_C     , "Top right soft"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::BUT_D     , "Bottom right soft"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::ROT_LEFT  , "Adjust left"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::ROT_RIGHT , "Adjust right"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::TRIGGER1_A, "Trigger1-A"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::TRIGGER1_B, "Trigger1-B"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::TRIGGER2_A, "Trigger2-A"));
    _keypress_action.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::TRIGGER2_B, "Trigger2-B"));

    // Add the possible options to the display list, and select the currently saved option
    uint8_t current_setting = 0;
   _keypress_action_list->clear_options();
    for (uint8_t n=0; n < _keypress_action.size(); n++)
    {
        _keypress_action_list->add_option(_keypress_action[n].text);
        if (_keypress_action[n].id == current_action)
        {
            current_setting = n;
        }
    }

    _keypress_action_list->set_selected(current_setting);
}
