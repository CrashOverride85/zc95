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

#include "COptionsList.h"



COptionsList::COptionsList(CDisplay* display, struct display_area area)
{
    _display = display;
    _area = area;
}

void COptionsList::add_option(std::string option_text)
{
    _options.push_back(option_t(option_text, -1));
}

void COptionsList::add_option(std::string option_text, int id)
{
    _options.push_back(option_t(option_text, id));
}

void COptionsList::clear_options()
{
    _options.clear();
    _current_selection = 0;
}

void COptionsList::set_selected(uint8_t selection)
{
    if (selection < _options.size())
    {
        _current_selection = selection;
    }
    else
    {
        printf("COptionsList::set_selected: Error - passed invalid selection %d (max %d)\n", selection, _options.size());
    }
}

void COptionsList::draw()
{
    int16_t centre_y;
    int16_t text_x;

    //hagl_draw_rectangle(_area.x0, _area.y0, _area.x1, _area.y1, hagl_color(0x00, 0xFF, 0x00)); // DEBUG only. draw a box around the passed in area

    // Work out how many options we can show either side of the currently selected option
    centre_y = ((_area.y1-_area.y0)/2) - (_display->get_font_height()/2);
    int option_height = (_display->get_font_height()+2);
    int option_count = (centre_y/option_height);
    
    text_x = _area.x0+1;

    for (int8_t option=-option_count; option <= option_count; option++)
    {
        uint8_t brightness;
        hagl_color_t colour;
        
        // Make the currently selected option white
        if (option == 0)
        {
            colour = hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF);
        }

        // Make the options either side decrease in brightness the further away from the selected option you get.
        // Also make the drop in brightness from the selected option to the next option either side grater than 
        // the drop between the following options to make it more obvious what's currently selected.
        brightness = 200 / (1 + abs(option));
        colour = hagl_color(_display->get_hagl_backed(), brightness, brightness, brightness);

        // Draw options
        if 
        (
            ((_current_selection+option) >= 0) &&
            ((_current_selection+option) < (int8_t)_options.size())
        )
        {
            _display->put_text(_options[_current_selection+option].DisplayText(), text_x, _area.y0 + centre_y + (option_height * option), colour);
        }
    }
}

void COptionsList::up()
{
    if (_current_selection > 0)
        _current_selection--;
}

void COptionsList::down()
{
    if (_current_selection < (_options.size()-1))
        _current_selection++;
}

uint8_t COptionsList::get_current_selection()
{
    return _current_selection;
}

int COptionsList::get_current_selection_id()
{
    return _options[_current_selection].Id();
}
