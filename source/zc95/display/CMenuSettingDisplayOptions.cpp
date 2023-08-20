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

#include "CMenuSettingDisplayOptions.h"
#include "CMenuSettingLedBrightnes.h"
#include "CMenuSettingPowerLevelDisplay.h"

CMenuSettingDisplayOptions::CMenuSettingDisplayOptions(
        CDisplay* display, 
        CSavedSettings *saved_settings)
{
    printf("CMenuSettingDisplayOptions() \n");
    _display = display;
    _saved_settings = saved_settings;
    _exit_menu = false;
    _settings_list = new COptionsList(display, display->get_display_area());
}

CMenuSettingDisplayOptions::~CMenuSettingDisplayOptions()
{
    printf("~CMenuSettingDisplayOptions() \n");
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
}

void CMenuSettingDisplayOptions::button_pressed(Button button)
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
                _last_selection = _settings_list->get_current_selection();
                show_selected_setting();
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                _saved_settings->save();
                break;

            case Button::C: // "Up"
                _settings_list->up();
                break;

            case Button::D: // "Down"
                _settings_list->down();
                break;
        }
    }
}

void CMenuSettingDisplayOptions::show_selected_setting()
{
    switch (_settings[_settings_list->get_current_selection()].id)
    {
        case setting_id::LED_BRIGHTNESS:
            set_active_menu(new CMenuSettingLedBrightnes(_display, _saved_settings));
            break;

        case setting_id::POWER_LEVEL_DISPLAY:
            set_active_menu(new CMenuSettingPowerLevelDisplay(_display, _saved_settings));
            break;
    }
}

void CMenuSettingDisplayOptions::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

 void CMenuSettingDisplayOptions::draw()
 {
    _settings_list->draw();
 }

void CMenuSettingDisplayOptions::show()
{
    _display->set_option_a("Select");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _settings.clear();

    _settings.push_back(CMenuSettingDisplayOptions::setting(setting_id::LED_BRIGHTNESS,      "LED brightness     "));
    _settings.push_back(CMenuSettingDisplayOptions::setting(setting_id::POWER_LEVEL_DISPLAY, "Power level display"));
    
   _settings_list->clear_options();
    for (std::vector<CMenuSettingDisplayOptions::setting>::iterator it = _settings.begin(); it != _settings.end(); it++)
    {
        _settings_list->add_option((*it).text);
    }

    // If we've already been in a setting menu and come back to this menu, pre-select that setting, instead
    // of going back to the top of the list
    if (_last_selection > 0)
    {
        _settings_list->set_selected(_last_selection);
    }

    _exit_menu = false;
}
