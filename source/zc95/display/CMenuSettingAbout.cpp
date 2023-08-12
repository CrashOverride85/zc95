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

CMenuSettingAbout::CMenuSettingAbout(CDisplay* display, CGetButtonState *buttons, CHwCheck *hwCheck)
{
    printf("CMenuSettingAbout() \n");
    _display = display;
    _buttons = buttons;
    _hwCheck = hwCheck;
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
    uint8_t line = 1;
    display_area disp_area = _display->get_display_area();
    std::string zc624_ver = _hwCheck->get_zc624_version();

    put_text_line("Firmware versions:", disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));

    put_text_line("ZC95              ", disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    put_text_line(kGitHash            , disp_area.x0+5, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0x99, 0x99, 0x99));
    
    line++;
    put_text_line("ZC624 output      ", disp_area.x0+2, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    put_text_line(zc624_ver           , disp_area.x0+5, disp_area.y0, line++, hagl_color(_display->get_hagl_backed(), 0x99, 0x99, 0x99));
}

void CMenuSettingAbout::put_text_line(std::string text, int16_t x, int16_t y, uint8_t line, hagl_color_t colour)
{
    _display->put_text(text, x, y + (line * 10), colour);
}

void CMenuSettingAbout::show()
{
    _display->set_option_a(" ");
    _display->set_option_b("Back");
    _display->set_option_c(" ");
    _display->set_option_d(" ");

    _exit_menu = false;
}
