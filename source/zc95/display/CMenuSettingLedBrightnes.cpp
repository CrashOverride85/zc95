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

#include "CMenuSettingLedBrightnes.h"


CMenuSettingLedBrightnes::CMenuSettingLedBrightnes(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings)
{
    printf("CMenuSettingLedBrightnes() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;

    _bar_graph = new CHorzBarGraph(_display);

    display_area disp_area = _display->get_display_area();
    _bar_graph_area.x0 = disp_area.x0;
    _bar_graph_area.x1 = disp_area.x1 - 2;
    _bar_graph_area.y0 = disp_area.y1 - ((disp_area.y1 - disp_area.y0)/2);
    _bar_graph_area.y1 = disp_area.y1;

    _led_brightness = _saved_settings->get_led_brightness();
}

CMenuSettingLedBrightnes::~CMenuSettingLedBrightnes()
{
    printf("~CMenuSettingLedBrightnes() \n");
    if (_bar_graph)
    {
        delete _bar_graph;
        _bar_graph = NULL;
    }
}

void CMenuSettingLedBrightnes::button_pressed(Button button)
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
                _saved_settings->save();
                break;

            default:
                break;
        }
    }
}

void CMenuSettingLedBrightnes::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change >= 1)
        {
            if (_led_brightness < 100)
            {
                _led_brightness++;
            }
        }
        else if (change <= -1)
        {
            if (_led_brightness > 1)
            {
                _led_brightness--;
            }
        }
        
        _saved_settings->set_led_brightness(_led_brightness);
    }
}

void CMenuSettingLedBrightnes::draw()
{
    display_area disp_area = _display->get_display_area();
    _display->put_text("LED Brightness", disp_area.x0+2, disp_area.y0 + 10, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));

    hagl_color_t bar_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);

    _bar_graph->draw_horz_bar_graph( _bar_graph_area, 1, 100, _led_brightness, "%", bar_colour);
}

void CMenuSettingLedBrightnes::show()
{
    _display->set_option_a(" ");
    _display->set_option_b("Back");
    _display->set_option_c(" ");
    _display->set_option_d(" ");

    _exit_menu = false;
}


