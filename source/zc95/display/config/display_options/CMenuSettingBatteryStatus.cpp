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

#include "CMenuSettingBatteryStatus.h"
#include "../CMenuSettings.h"
#include "../../../CDebugOutput.h"
#include "../../../common/zc95_config.h"

CMenuSettingBatteryStatus::CMenuSettingBatteryStatus(CDisplay* display, CSavedSettings *saved_settings)
{
    printf("CMenuSettingBatteryStatus()\n");
    _display = display;
    _saved_settings = saved_settings;

    _exit_menu = false;

    _setting_choice_area = display->get_display_area();
    _setting_choice_area.y0 = _setting_choice_area.y1 - ((_setting_choice_area.y1-_setting_choice_area.y0)/2);
    _settings_choice_list = new COptionsList(display, _setting_choice_area);
}

CMenuSettingBatteryStatus::~CMenuSettingBatteryStatus()
{
    printf("~CMenuSettingBatteryStatus()\n");

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_settings_choice_list)
    {
        delete _settings_choice_list;
        _settings_choice_list = NULL;
    }
}

void CMenuSettingBatteryStatus::button_pressed(Button button)
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
    }
}

void CMenuSettingBatteryStatus::adjust_rotary_encoder_change(int8_t change)
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

        save_setting();
    }
}

void CMenuSettingBatteryStatus::save_setting()
{
    _saved_settings->set_battery_status((CSavedSettings::battery_status_t)_settings_choice_list->get_current_selection_id());
}

void CMenuSettingBatteryStatus::draw()
{
    display_area disp_area = _display->get_display_area();
    _display->put_text("Battery status", disp_area.x0+2, disp_area.y0 + 10, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));

    hagl_color_t rect_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);

    hagl_fill_rectangle(_display->get_hagl_backed(), _setting_choice_area.x0, _setting_choice_area.y0,
                        _setting_choice_area.x1, _setting_choice_area.y1, rect_colour);

    _settings_choice_list->draw();
}

void CMenuSettingBatteryStatus::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("");
    _display->set_option_d("");

    _exit_menu = false;
    set_options_on_multi_choice_list();
}

void CMenuSettingBatteryStatus::set_options_on_multi_choice_list()
{
    _settings_choice_list->clear_options();

    _settings_choice_list->add_option("Percentage", (uint8_t)CSavedSettings::battery_status_t::PERCENTAGE);
    _settings_choice_list->add_option("Voltage", (uint8_t)CSavedSettings::battery_status_t::VOLTAGE);

    uint8_t current_choice_id = (uint8_t)_saved_settings->get_battery_status();

    _settings_choice_list->set_selected_by_id(current_choice_id);
}
