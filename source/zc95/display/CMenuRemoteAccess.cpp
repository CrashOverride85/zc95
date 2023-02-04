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

#include "CMenuRemoteAccess.h"
#include "CMenuApMode.h"
#include "CDisplayMessage.h"

CMenuRemoteAccess::CMenuRemoteAccess(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CWifi *wifi, CAnalogueCapture *analogueCapture, CRoutineOutput *routine_output)
{
    printf("CMenuRemoteAccess() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _wifi = wifi;
    _analogueCapture = analogueCapture;
    _routine_output = routine_output;

    _exit_menu = false;
    _options_list = new COptionsList(display, display->get_display_area());
}

CMenuRemoteAccess::~CMenuRemoteAccess()
{
    printf("~CMenuRemoteAccess() \n");
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
}

void CMenuRemoteAccess::button_pressed(Button button)
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
                _last_selection = _options_list->get_current_selection();
                show_selected_setting();
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
    }
}

void CMenuRemoteAccess::show_selected_setting()
{
    switch (_options[_options_list->get_current_selection()].id)
    {
        case option_id::AP_MODE:
            set_active_menu(new CMenuApMode(_display, _buttons, _saved_settings, _wifi, _analogueCapture));
            break;

        case option_id::CONNECT_WIFI:
            set_active_menu(new CMenuRemoteAccessConnectWifi(_display, _buttons, _saved_settings, _wifi, _routine_output));
            break;

        case option_id::CLEAR_SAVED_CREDS:
            _saved_settings->clear_wifi_credentials();
            _saved_settings->save();
            set_active_menu(new CDisplayMessage(_display, _buttons, "Saved WiFi credentials cleared"));
            break;

        case option_id::REGEN_AP_PSK:
            _saved_settings->clear_saved_ap_psk();
            _saved_settings->save();
            set_active_menu(new CDisplayMessage(_display, _buttons, "New PSK for AP mode will be generated when AP mode next started"));
            break;
    }
}

void CMenuRemoteAccess::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

 void CMenuRemoteAccess::draw()
 {
    _options_list->draw();
 }

void CMenuRemoteAccess::show()
{
    _display->set_option_a("Select");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _options.clear();
    _options.push_back(CMenuRemoteAccess::option(option_id::CONNECT_WIFI     ,  "Connect to WiFi"));
    _options.push_back(CMenuRemoteAccess::option(option_id::AP_MODE          ,  "Config Wifi/AP mode"));
    _options.push_back(CMenuRemoteAccess::option(option_id::CLEAR_SAVED_CREDS,  "Clear WiFi creds."));
    _options.push_back(CMenuRemoteAccess::option(option_id::REGEN_AP_PSK     ,  "New AP password"));
    
   _options_list->clear_options();
    for (std::vector<CMenuRemoteAccess::option>::iterator it = _options.begin(); it != _options.end(); it++)
    {
        _options_list->add_option((*it).text);
    }

    // If we've already been in a setting menu and come back to this menu, pre-select that setting, instead
    // of going back to the top of the list
    if (_last_selection > 0)
    {
        _options_list->set_selected(_last_selection);
    }

    _exit_menu = false;
}
