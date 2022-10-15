/*
 * ZC95
 * Copyright (C) 2021  CrashOverride85
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

#include "CMenuSettings.h"
#include "CMenuChannelConfig.h"
#include "CMenuCollarConfig.h"
#include "CMenuSettingLedBrightnes.h"
#include "CMenuSettingPowerStep.h"
#include "CMenuSettingRampUpTime.h"
#include "CMenuSettingAbout.h"
#include "CMenuSettingAudio.h"
#include "CMenuSettingHardware.h"
#include "../core1/routines/CRoutine.h"

CMenuSettings::CMenuSettings(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CRoutineOutput *routine_output, CHwCheck *hwCheck, CAudio *audio)
{
    printf("CMenuSettings() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _exit_menu = false;
    _routine_output = routine_output;
    _settings_list = new COptionsList(display, display->get_display_area());
    _hwCheck = hwCheck;
    _audio = audio;
}

CMenuSettings::~CMenuSettings()
{
    printf("~CMenuSettings() \n");
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

void CMenuSettings::button_pressed(Button button)
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

void CMenuSettings::show_selected_setting()
{
    switch (_settings[_settings_list->get_current_selection()].id)
    {
        case setting_id::CHANNEL_CONFIG:
            set_active_menu(new CMenuChannelConfig(_display, _buttons, _saved_settings, _routine_output));
            break;

        case setting_id::COLLAR_CONFIG:
            set_active_menu(new CMenuCollarConfig(_display, _buttons, _saved_settings, _routine_output));
            break;

        case setting_id::LED_BRIGHTNESS:
            set_active_menu(new CMenuSettingLedBrightnes(_display, _buttons, _saved_settings));
            break;

        case setting_id::POWER_STEP:
            set_active_menu(new CMenuSettingPowerStep(_display, _buttons, _saved_settings));
            break;

        case setting_id::RAMP_UP_TIME:
            set_active_menu(new CMenuSettingRampUpTime(_display, _buttons, _saved_settings));
            break;

        case setting_id::AUDIO:
            set_active_menu(new CMenuSettingAudio(_display, _buttons, _audio, _saved_settings));
            break;

        case setting_id::HARDWARE:
            set_active_menu(new CMenuSettingHardware(_display, _buttons, _saved_settings, _routine_output, _audio));
            break;

        case setting_id::ABOUT:
            set_active_menu(new CMenuSettingAbout(_display, _buttons, _hwCheck));
            break;
    }
}

void CMenuSettings::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

 void CMenuSettings::draw()
 {
    _settings_list->draw();
 }

void CMenuSettings::show()
{
    _display->set_option_a("Select");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _settings.clear();
    _settings.push_back(CMenuSettings::setting(setting_id::CHANNEL_CONFIG, "Channel config"));
    _settings.push_back(CMenuSettings::setting(setting_id::COLLAR_CONFIG,  "Collar config"));
    _settings.push_back(CMenuSettings::setting(setting_id::LED_BRIGHTNESS, "LED brightness"));
    _settings.push_back(CMenuSettings::setting(setting_id::RAMP_UP_TIME,   "Ramp up time"));
    
    if (_audio->get_audio_hardware_state() != audio_hardware_state_t::NOT_PRESENT)
        _settings.push_back(CMenuSettings::setting(setting_id::AUDIO,          "Audio input"));
    
    _settings.push_back(CMenuSettings::setting(setting_id::HARDWARE,       "Hardware config"));
    _settings.push_back(CMenuSettings::setting(setting_id::ABOUT,          "About"));  
    
   _settings_list->clear_options();
    for (std::vector<CMenuSettings::setting>::iterator it = _settings.begin(); it != _settings.end(); it++)
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
