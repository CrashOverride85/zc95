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

#include "CMenuSettingAbout.h"
#include "../git_version.h"

CMenuSettingAbout::CMenuSettingAbout(CDisplay* display, CGetButtonState *buttons)
{
    printf("CMenuSettingAbout() \n");
    _display = display;
    _buttons = buttons;
}

CMenuSettingAbout::~CMenuSettingAbout()
{
    printf("~CMenuSettingAbout() \n");
}

void CMenuSettingAbout::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::B: // "Back"
                _exit_menu = true;
                break;

            default:
                break;
        }
    }
}

void CMenuSettingAbout::adjust_rotary_encoder_change(int8_t change)
{

}

void CMenuSettingAbout::draw()
{
    display_area disp_area = _display->get_display_area();
    _display->put_text("Firmware version: ", disp_area.x0+2, disp_area.y0 + 10, hagl_color(0xFF, 0xFF, 0xFF));
    _display->put_text(kGitHash            , disp_area.x0+5, disp_area.y0 + 20, hagl_color(0x99, 0x99, 0x99));    
}

void CMenuSettingAbout::show()
{
    _display->set_option_a(" ");
    _display->set_option_b("Back");
    _display->set_option_c(" ");
    _display->set_option_d(" ");

    _exit_menu = false;
}
