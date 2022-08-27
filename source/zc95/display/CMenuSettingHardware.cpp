/*
 * ZC95
 * Copyright (C) 2022  CrashOverride85
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

#include "CMenuSettingHardware.h"
#include "CMenuSettings.h"
#include "../CDebugOutput.h"
#include "../config.h"


CMenuSettingHardware::CMenuSettingHardware(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CRoutineOutput *routine_output, CAudio *audio)
{
    printf("CMenuSettingHardware()\n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _routine_output = routine_output;

    _exit_menu = false;
    display_area area = display->get_display_area();
    area.y1 = area.y0 + ((area.y1-area.y0)/2);
    _settings_list = new COptionsList(display, area);

    _setting_choice_area = display->get_display_area();
    _setting_choice_area.y0 = _setting_choice_area.y1 - ((_setting_choice_area.y1-_setting_choice_area.y0)/2);
    _settings_choice_list = new COptionsList(display, _setting_choice_area);
    _audio = audio;
}

CMenuSettingHardware::~CMenuSettingHardware()
{
    printf("~CMenuSettingHardware()\n");

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

void CMenuSettingHardware::button_pressed(Button button)
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

void CMenuSettingHardware::adjust_rotary_encoder_change(int8_t change)
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

void CMenuSettingHardware::save_setting(uint8_t setting_menu_index, uint8_t choice_menu_index)
{
    setting_t setting = _settings[setting_menu_index];
    setting_t choice_id = _setting_choices[choice_menu_index];

    switch (setting.id)
    {
        case setting_id::AUDIO:
            _saved_settings->set_audio_setting((CSavedSettings::setting_audio)choice_id.id);
            break;

        case setting_id::DEBUG:
            _saved_settings->set_debug_dest((CSavedSettings::setting_debug)choice_id.id);
            CDebugOutput::set_debug_destination_from_settings(_saved_settings);
            break;

        case setting_id::AUX_USE:
            _saved_settings->set_aux_port_use((CSavedSettings::setting_aux_port_use)choice_id.id);
            if (choice_id.id == (uint8_t)CSavedSettings::setting_aux_port_use::AUDIO)
                _audio->audio_input_enable(true);
            else
                _audio->audio_input_enable(false);
            break;
    }
}

void CMenuSettingHardware::draw()
{
    _settings_list->draw();

    color_t rect_colour = hagl_color(0x00, 0x00, 0xFF);

    hagl_fill_rectangle(_setting_choice_area.x0, _setting_choice_area.y0,
                        _setting_choice_area.x1, _setting_choice_area.y1,
                        rect_colour);

    _settings_choice_list->draw();
}

void CMenuSettingHardware::show()
{
    _display->set_option_a(" ");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _settings.clear();
    _settings.push_back(CMenuSettingHardware::setting_t(setting_id::AUDIO  , "Audio"));
    _settings.push_back(CMenuSettingHardware::setting_t(setting_id::DEBUG  , "Debug output"));
    _settings.push_back(CMenuSettingHardware::setting_t(setting_id::AUX_USE, "Aux port use"));

    _settings_list->clear_options();
    for (std::vector<CMenuSettingHardware::setting_t>::iterator it = _settings.begin(); it != _settings.end(); it++)
    {
        _settings_list->add_option((*it).text);
    }

    _exit_menu = false;
    set_options_on_multi_choice_list(0);
}

void CMenuSettingHardware::set_options_on_multi_choice_list(uint8_t setting_id)
{
    _setting_choices.clear();
    uint8_t current_choice_id = 0;

    switch (setting_id)
    {
        case setting_id::AUDIO:
            _setting_choices.push_back(CMenuSettingHardware::setting_t((uint8_t)CSavedSettings::setting_audio::AUTO   , "Auto"        ));
            _setting_choices.push_back(CMenuSettingHardware::setting_t((uint8_t)CSavedSettings::setting_audio::NO_GAIN, "On (no gain)"));
            _setting_choices.push_back(CMenuSettingHardware::setting_t((uint8_t)CSavedSettings::setting_audio::OFF    , "Off"         ));
            current_choice_id = (uint8_t)_saved_settings->get_audio_setting();
            break;

        case setting_id::DEBUG:
            _setting_choices.push_back(CMenuSettingHardware::setting_t((uint8_t)CSavedSettings::setting_debug::ACC_PORT, "Accessory port"));
            _setting_choices.push_back(CMenuSettingHardware::setting_t((uint8_t)CSavedSettings::setting_debug::AUX_PORT, "Aux port"      ));
            _setting_choices.push_back(CMenuSettingHardware::setting_t((uint8_t)CSavedSettings::setting_debug::OFF     , "Off"           ));
            current_choice_id = (uint8_t)_saved_settings->get_debug_dest();
            break;

        case setting_id::AUX_USE:
            _setting_choices.push_back(CMenuSettingHardware::setting_t((uint8_t)CSavedSettings::setting_aux_port_use::AUDIO,  "Audio input"));
            _setting_choices.push_back(CMenuSettingHardware::setting_t((uint8_t)CSavedSettings::setting_aux_port_use::SERIAL, "Serial I/O" ));
            current_choice_id = (uint8_t)_saved_settings->get_aux_port_use();
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
    _settings_choice_list->set_selected(current_setting);
}
