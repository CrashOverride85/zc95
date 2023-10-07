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

#include "CMenuBluetooth.h"
#include "CMenuBluetoothScan.h"
#include "CMenuBluetoothTest.h"
#include "../CDebugOutput.h"
#include "../config.h"


CMenuBluetooth::CMenuBluetooth(CDisplay* display, CSavedSettings *saved_settings, CBluetooth *bluetooth)
{
    printf("CMenuBluetooth()\n");
    _display = display;
    _saved_settings = saved_settings;
    _bluetooth = bluetooth;

    _exit_menu = false;
    display_area area = display->get_display_area();
    area.y1 = area.y0 + ((area.y1-area.y0)/2);
    _settings_list = new COptionsList(display, area);

    _setting_choice_area = display->get_display_area();
    _setting_choice_area.y0 = _setting_choice_area.y1 - ((_setting_choice_area.y1-_setting_choice_area.y0)/2);
    _settings_choice_list = new COptionsList(display, _setting_choice_area);
}

CMenuBluetooth::~CMenuBluetooth()
{
    printf("~CMenuBluetooth()\n");

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_settings_list)
    {
        delete _settings_list;
        _settings_list = NULL;
    }

    if (_settings_choice_list)
    {
        delete _settings_choice_list;
        _settings_choice_list = NULL;
    }
}

void CMenuBluetooth::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        if (button == Button::A)
        {
            if (get_current_setting().id ==  CMenuBluetooth::setting_id::SCAN)
            {
                set_active_menu(new CMenuBluetoothScan(_display, _saved_settings, _bluetooth));
            }
            else if (get_current_setting().id ==  CMenuBluetooth::setting_id::TEST)
            {
                set_active_menu(new CMenuBluetoothTest(_display, _bluetooth));
            }
        }

        if (button == Button::B) // "Back"
        {
            _exit_menu = true;
        }

        if (button == Button::C) // "Up"
        {
            _settings_list->up();
            set_options_on_multi_choice_list(_settings_list->get_current_selection());
        }

        if (button == Button::D) // "Down"
        {
            _settings_list->down();
            set_options_on_multi_choice_list(_settings_list->get_current_selection());
        }
    }
}

void CMenuBluetooth::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change >= 1)
        {
            _settings_choice_list->down();
        }
        else if (change <= -1)
        {
            _settings_choice_list->up();
        }

        save_setting(_settings_list->get_current_selection(), _settings_choice_list->get_current_selection());
    }
}

void CMenuBluetooth::save_setting(uint8_t setting_menu_index, uint8_t choice_menu_index)
{
    setting_t setting = _settings[setting_menu_index];
    setting_t choice_id = _setting_choices[choice_menu_index];

    switch (setting.id)
    {
        case setting_id::ENABLED:
            _saved_settings->set_bluethooth_enabled(choice_id.id);
            break;
    }
}

void CMenuBluetooth::draw()
{
    _settings_list->draw();
    bool choice_list_required = false;

    setting_t setting = get_current_setting();
    switch (setting.id)
    {
        case setting_id::ENABLED:
            _display->set_option_a(" ");
            choice_list_required = true;
            break;

        case setting_id::SCAN:
            _display->set_option_a("Select");
            choice_list_required = false;
            // TODO
            break;
    }

    // Show list of options for the selected setting in blue box at bottom of screen
    if (choice_list_required)
    {
        hagl_color_t rect_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);
        hagl_fill_rectangle(_display->get_hagl_backed(), _setting_choice_area.x0, _setting_choice_area.y0,
                            _setting_choice_area.x1, _setting_choice_area.y1, rect_colour);
        _settings_choice_list->draw();
    }
}

void CMenuBluetooth::show()
{
    _display->set_option_a(" ");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _settings.clear();
    _settings.push_back(CMenuBluetooth::setting_t(setting_id::ENABLED, "Enabled"  ));
    _settings.push_back(CMenuBluetooth::setting_t(setting_id::SCAN   , "Scan/Pair"));
    _settings.push_back(CMenuBluetooth::setting_t(setting_id::TEST   , "Test"));

    _settings_list->clear_options();
    for (std::vector<CMenuBluetooth::setting_t>::iterator it = _settings.begin(); it != _settings.end(); it++)
    {
        _settings_list->add_option((*it).text);
    }

    _exit_menu = false;
    set_options_on_multi_choice_list(0);
}

void CMenuBluetooth::set_options_on_multi_choice_list(uint8_t setting_id)
{
    _setting_choices.clear();
    uint8_t current_choice_id = 0;

    switch (setting_id)
    {
        case setting_id::ENABLED:
            _setting_choices.push_back(CMenuBluetooth::setting_t(0, "Off"));
            _setting_choices.push_back(CMenuBluetooth::setting_t(1, "On" ));
            current_choice_id = (uint8_t)_saved_settings->get_bluethooth_enabled();
            break;
    }

    // Add the possible options to the display list, and select the currently saved option
    uint8_t current_setting = 0;
   _settings_choice_list->clear_options();
    for (uint8_t n=0; n < _setting_choices.size(); n++)
    {
        _settings_choice_list->add_option(_setting_choices[n].text);
        if (_setting_choices[n].id == current_choice_id)
        {
            current_setting = n;
        }
    }
    if (_setting_choices.size() > 0)
        _settings_choice_list->set_selected(current_setting);
}

CMenuBluetooth::setting_t CMenuBluetooth::get_current_setting()
{
    return _settings[_settings_list->get_current_selection()];
}
