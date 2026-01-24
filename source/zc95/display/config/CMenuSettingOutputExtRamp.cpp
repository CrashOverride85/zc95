/*
 * ZC95
 * Copyright (C) 2026  CrashOverride85
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

#include "CMenuSettingOutputExtRamp.h"
#include "CMenuSettings.h"
#include "../CDebugOutput.h"
#include "../config.h"


CMenuSettingOutputExtRamp::CMenuSettingOutputExtRamp(CDisplay* display, IHal *hal, CSavedSettings *saved_settings)
{
    printf("CMenuSettingOutputExtRamp()\n");
    _display = display;
    _hal = hal;
    _saved_settings = saved_settings;

    _exit_menu = false;
    display_area area = display->get_display_area();
    area.y1 = area.y0 + ((area.y1-area.y0)/2);
    _settings_list = new COptionsList(display, area);

    _setting_choice_area = display->get_display_area();
    _setting_choice_area.y0 = _setting_choice_area.y1 - ((_setting_choice_area.y1-_setting_choice_area.y0)/2);
    _setting_choice_area.y1 -= 10;
    _setting_choice_area.x1 -= 2;
    _settings_choice_list = new COptionsList(display, _setting_choice_area);

    _duration_area = display->get_display_area();
    _duration_area.y0 = _setting_choice_area.y1;

    _bar_graph = new CHorzBarGraph(_display);
}

CMenuSettingOutputExtRamp::~CMenuSettingOutputExtRamp()
{
    printf("~CMenuSettingOutputExtRamp()\n");

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

    if (_bar_graph)
    {
        delete _bar_graph;
        _bar_graph = NULL;
    }
}

void CMenuSettingOutputExtRamp::button_pressed(Button button)
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
            set_options_for_setting((setting_id_t)_settings_list->get_current_selection());
        }

        if (button == Button::D) // "Down"
        {
            _settings_list->down();
            set_options_for_setting((setting_id_t)_settings_list->get_current_selection());
        }
    }
}

void CMenuSettingOutputExtRamp::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (get_setting_kind(get_currently_selected_setting_id()) == setting_kind_t::MULTI_CHOICE)
        {
            if (change >= 1)
            {
                _settings_choice_list->down();
            }
            else if (change <= -1)
            {
                _settings_choice_list->up();
            }
        }
        else if (get_setting_kind(get_currently_selected_setting_id()) == setting_kind_t::MIN_MAX)
        {
            if (change >= 1)
            {
                if (_min_max_value < _min_max_value_max)
                {
                    _min_max_value++;
                }
            }
            else if (change <= -1)
            {
                if (_min_max_value > _min_max_value_min)
                {
                    _min_max_value--;
                }
            }
        }

        save_setting(_settings_list->get_current_selection(), _settings_choice_list->get_current_selection());
    }
}

void CMenuSettingOutputExtRamp::save_setting(uint8_t setting_menu_index, uint8_t choice_menu_index)
{
    setting_t setting = _settings[setting_menu_index];
    setting_t choice_id = _setting_choices[choice_menu_index];

    switch (setting.id)
    {
        case setting_id_t::RAMP_LEVEL:
            _saved_settings->set_extended_ramp_level(_min_max_value);
            break;

        case setting_id_t::RAMP_TIME:
            _saved_settings->set_extended_ramp_time_seconds(_min_max_value);
            break;
    }
}

void CMenuSettingOutputExtRamp::draw()
{
    _settings_list->draw();

    switch (get_setting_kind(get_currently_selected_setting_id()))
    {
        case setting_kind_t::MULTI_CHOICE:
        {
            hagl_color_t blue = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);

            hagl_fill_rectangle(_display->get_hagl_backed(), _setting_choice_area.x0, _setting_choice_area.y0,
                                _setting_choice_area.x1, _setting_choice_area.y1, blue);

            _settings_choice_list->draw();
        }
            break;

        case setting_kind_t::MIN_MAX:
        {
            hagl_color_t bar_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);
            _bar_graph->draw_horz_bar_graph(_setting_choice_area, _min_max_value_min, _min_max_value_max, _min_max_value, _min_max_uom, bar_colour);
        }
            break;
    };

    // Show calculated ramp duration at bottom
    uint8_t steps = 100 - _saved_settings->get_extended_ramp_level();
    int duration_seconds = steps * _saved_settings->get_extended_ramp_time_seconds();
    float duration_minutes = (float)duration_seconds / (float)60;
    std::ostringstream oss;
    oss << "Dur.: " << std::fixed << std::setprecision(1) << duration_minutes << " mins";
    _display->put_text(oss.str(), _duration_area.x0, _duration_area.y1-_display->get_font_height(), hagl_color(_display->get_hagl_backed(), 0x55, 0x55, 0x55));
}

void CMenuSettingOutputExtRamp::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _settings.clear();
    _settings.push_back(CMenuSettingOutputExtRamp::setting_t(setting_id_t::RAMP_LEVEL, "Start level"));
    _settings.push_back(CMenuSettingOutputExtRamp::setting_t(setting_id_t::RAMP_TIME , "Time per p.p."));

    _settings_list->clear_options();
    for (std::vector<CMenuSettingOutputExtRamp::setting_t>::iterator it = _settings.begin(); it != _settings.end(); it++)
    {
        _settings_list->add_option((*it).text);
    }

    _exit_menu = false;
    set_options_for_setting(setting_id_t::RAMP_LEVEL);
}

void CMenuSettingOutputExtRamp::set_options_for_setting(setting_id_t setting_id)
{
    _setting_choices.clear();
    uint8_t current_choice_id = 0;

    switch (setting_id)
    {
        case setting_id_t::RAMP_LEVEL:
            _min_max_value_min = 1;
            _min_max_value_max = 100; 
            _min_max_uom = "%";
            _min_max_value = _saved_settings->get_extended_ramp_level();
            break;

        case setting_id_t::RAMP_TIME:
            _min_max_value_min = 1;
            _min_max_value_max = 200;
            _min_max_uom = "sec";
            _min_max_value = _saved_settings->get_extended_ramp_time_seconds();
            break;
    }

    if (get_setting_kind(setting_id) == setting_kind_t::MULTI_CHOICE)
    {
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
}

CMenuSettingOutputExtRamp::setting_kind_t CMenuSettingOutputExtRamp::get_setting_kind(setting_id_t setting_id)
{
    switch (setting_id)
    {
        case setting_id_t::RAMP_LEVEL:
        case setting_id_t::RAMP_TIME:
        default:
            return setting_kind_t::MIN_MAX;        
    };
}

CMenuSettingOutputExtRamp::setting_id_t CMenuSettingOutputExtRamp::get_currently_selected_setting_id()
{
    return (setting_id_t)_settings[_settings_list->get_current_selection()].id;
}