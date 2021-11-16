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

#include "CHorzBarGraph.h"

/*
 * Draw a horizontal bar graph/adjuster thingy
 */

CHorzBarGraph::CHorzBarGraph(CDisplay *display)
{
    _display = display;
}

// int16_t x, int16_t y, uint8_t width, uint8_t height,
void CHorzBarGraph::draw_horz_bar_graph(display_area area, int16_t min_val, int16_t max_val, int16_t current_val, std::string UoM, color_t bar_colour)
{
    if (current_val > max_val)
        current_val = max_val;
    if (current_val < min_val)
        current_val = min_val;

    if (max_val == 0 || min_val > max_val || max_val-min_val == 0)
        return;

    hagl_draw_rectangle(area.x0, area.y0, area.x1, area.y1-_display->get_font_height()-2, hagl_color(0xFF, 0xFF, 0xFF));

    int16_t width = area.x1 - area.x0;

    float current_val_pc = (((float)current_val - (float)min_val) / ((float)max_val-(float)min_val));
    int16_t bar_width = current_val_pc * (float)((width)-1);
  
    int16_t bar_height = (area.y1-_display->get_font_height()-2) - (area.y0) - 1;

    hagl_fill_rectangle(area.x0+1, area.y0+1, area.x0+bar_width, area.y0+bar_height, bar_colour);

    // min label
    _display->put_text(std::to_string(min_val), area.x0, area.y1-_display->get_font_height(), hagl_color(0, 0xFF, 0));

    // max label
    _display->put_text(std::to_string(max_val), area.x0+width-20, area.y1-_display->get_font_height(), hagl_color(0, 0xFF, 0));

    // current value label in middle of bar
    std::string value_str = std::to_string(current_val) + " " + UoM;
    uint16_t current_val_width = value_str.length() * _display->get_font_width();
    _display->put_text(value_str, area.x0+((width/2)-(current_val_width/2)), area.y0 + (bar_height/2) - (_display->get_font_height()/2), hagl_color(0xFF, 0xFF, 0xFF));
}
