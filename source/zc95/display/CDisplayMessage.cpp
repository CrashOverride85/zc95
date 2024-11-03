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

#include "CDisplayMessage.h"
#include <algorithm>

CDisplayMessage::CDisplayMessage(CDisplay* display, CGetButtonState *buttons, std::string message)
{
    printf("CDisplayMessage()\n");
    _display = display;
    _buttons = buttons;
    _display_string = word_wrap(message, 19);
    _disp_area = _display->get_display_area();

    _exit_menu = false;
}

CDisplayMessage::~CDisplayMessage()
{
    printf("~CDisplayMessage()\n");
    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }
}

void CDisplayMessage::button_pressed(Button button)
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

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C:
                break;

            case Button::D:
                break;

            case Button::ROT:
                break;
        }
    }
}

void CDisplayMessage::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

 void CDisplayMessage::draw()
 {
    // Try and show the message in the middle of the screen vertically
    uint line_count = std::count(_display_string.begin(), _display_string.end(), '\n') + 1;

    uint8_t font_height = _display->get_font_height();
    uint text_height = line_count * font_height; 

    uint display_area_centre = _disp_area.y0 + ((_disp_area.y1 - _disp_area.y0) / 2);
    int text_y0 = display_area_centre - (text_height/2);
    
    if (text_y0 < _disp_area.y0)
        text_y0 = _disp_area.y0; // this will happen if the message is too long (too many lines).  Probably better to show the beginning of the message, than the middle.

    _display->put_text(_display_string, 0, text_y0, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
 }

void CDisplayMessage::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("");
    _display->set_option_d("");

    _exit_menu = false;
}


/* Word wrap - taken from:
 * https://ideone.com/wai9da   and
 * https://cplusplus.com/forum/beginner/132223/
 */
std::string CDisplayMessage::word_wrap(std::string text, unsigned per_line)
{
    unsigned line_begin = 0;
 
    while (line_begin < text.size())
    {
        const unsigned ideal_end = line_begin + per_line ;
        unsigned line_end = ideal_end <= text.size() ? ideal_end : text.size()-1;
 
        if (line_end == text.size() - 1)
            ++line_end;
        else if (std::isspace(text[line_end]))
        {
            text[line_end] = '\n';
            ++line_end;
        }
        else    // backtrack
        {
            unsigned end = line_end;
            while ( end > line_begin && !std::isspace(text[end]))
                --end;
 
            if (end != line_begin)
            {
                line_end = end;
                text[line_end++] = '\n';
            }
            else
                text.insert(line_end++, 1, '\n');
        }
 
        line_begin = line_end;
    }
 
    return text;
}
 