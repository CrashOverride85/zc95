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

/* Display a message on screen, with a Back option and nothing else.
 * Word wraps the incoming message, but doesn't support scrolling.
 */

#include "CMenuSettingsBatteryReset.h"
#include "../CDisplayMessage.h"
#include "../../../globals.h"
#include <algorithm>

CMenuSettingsBatteryReset::CMenuSettingsBatteryReset(CDisplay* display, IHal* hal)
{
    printf("CMenuSettingsBatteryReset()\n");
    _display = display;
    _hal = hal;
    _exit_menu = false;
    _disp_area = _display->get_display_area();
}

CMenuSettingsBatteryReset::~CMenuSettingsBatteryReset()
{
    printf("~CMenuSettingsBatteryReset()\n");
    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }
}

void CMenuSettingsBatteryReset::button_pressed(Button button)
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
                break;

            case Button::B: // No/Back
                _exit_menu = true;
                break;

            case Button::C:
                break;

            case Button::D: // Yes/Reset
                printf("Clear fuel gauge learned data in eeprom\n");
                g_SavedSettings->fuel_gauge_invalidate_learned_data();
                g_SavedSettings->save();

                printf("Reset fuel gauge\n");
                _hal->power_management()->fuel_gauge_reset();

                set_active_menu(new CDisplayMessage(_display, _hal, "Battery stats reset"));
                _exit_menu = true;
                break;

            case Button::ROT:
                break;
        }
    }
}

void CMenuSettingsBatteryReset::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

 void CMenuSettingsBatteryReset::draw()
 {
    uint8_t line = 0;
    uint8_t font_height = _display->get_font_height();
    
    _display->put_text("Battery gauge will" , 0, _disp_area.y0 + (line++ * font_height), hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    _display->put_text("be inaccurate until", 0, _disp_area.y0 + (line++ * font_height), hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    _display->put_text("full discharge /"   , 0, _disp_area.y0 + (line++ * font_height), hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    _display->put_text("recharge cycle."    , 0, _disp_area.y0 + (line++ * font_height), hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    _display->put_text(""                   , 0, _disp_area.y0 + (line++ * font_height), hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    _display->put_text("Are you sure?"      , 0, _disp_area.y0 + (line++ * font_height), hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
 }

void CMenuSettingsBatteryReset::show()
{
    _display->set_option_a("");
    _display->set_option_b("No/Back");
    _display->set_option_c("");
    _display->set_option_d("Yes/RESET");
}
