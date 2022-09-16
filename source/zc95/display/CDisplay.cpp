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

#include "CDisplay.h"
#include "pico/stdlib.h"
#include <font6x9.h>
#include <fontx.h>
#include <string>

#include "../globals.h"
#include "../CTimingTest.h"

/*
 * Basic display management. Updates the screen, draws the soft button boxes + text and power level graphs.
 * Calls update() for the currently active menu for it to fill in the centre of the display.
 */

const uint8_t menu_bar_height = 18;   // should be even (soft button text)
const uint8_t status_bar_height = 9;  // battery level + mode at bottom
const uint8_t bar_width = 10;         // individual power level bar width 

CDisplay::CDisplay()
{
    _channel_1_max_power = 0; 
    _channel_2_max_power = 0; 
    _channel_3_max_power = 0; 
    _channel_4_max_power = 0;

    _last_update = 0;
    _current_menu = NULL;

    fontx_glyph_t glyph;
    fontx_glyph(&glyph, L'A', font6x9);

    _font_width = glyph.width;
    _font_height = glyph.height;
    _battery_percentage = 0;
    _active_pattern = "";
}

uint8_t CDisplay::get_font_width()
{
    return _font_width;
}

uint8_t CDisplay::get_font_height()
{
    return _font_height;
}

 void CDisplay::init()
 {
    hagl_init();
    hagl_clear_screen();
    // hagl_set_clip_window(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    update();
 }

 void CDisplay::update()
 {
    // Don't try to update the screen more than once every 100ms
    if (
        _update_required || 
        (time_us_64() - _last_update > (100*1000))
        )
    {
        _interuptable_section.start();
        
        _update_required = false;
        CTimingTest timing;
        hagl_clear_screen();

        if (_current_menu != NULL)
        {
            _current_menu->update();
        }

        draw_soft_buttons(); // 846us

        draw_bar_graphs(); // 489us
        
        draw_status_bar();
  
        hagl_flush(); // 8us

        _last_update = time_us_64();
        _interuptable_section.end();
    }
 }

void CDisplay::set_option_a(std::string text)
{
    _option_a = text;
}

void CDisplay::set_option_b(std::string text)
{
    _option_b = text;
}

void CDisplay::set_option_c(std::string text)
{
    _option_c = text;
}

void CDisplay::set_option_d(std::string text)
{
    _option_d = text;
}

// Max_power = 0-1000, as set on the front panel
// actual_power = scaled power as requested by running routine (actual_power should always be <= max_power)
void CDisplay::set_power_level(uint8_t channel, int16_t front_pannel_power, int16_t actual_power, int16_t maximum_power) // chan 0-3, others 0-1000
{
    if (front_pannel_power > 1000)
        front_pannel_power = 1000;
    if (front_pannel_power < 0)
        front_pannel_power = 0;

    if (actual_power > 1000)
        actual_power = 1000;
    if (actual_power < 0)
        actual_power = 0;

    if (maximum_power > 1000)
        maximum_power = 1000;
    if (maximum_power < 0)
        maximum_power = 0;

    switch (channel)
    {
        case 0:
            _channel_1_fp_power = front_pannel_power;
            _channel_1_actual_power = actual_power;
            _channel_1_max_power = maximum_power;
            break;

        case 1:
            _channel_2_fp_power = front_pannel_power;
            _channel_2_actual_power = actual_power;
            _channel_2_max_power = maximum_power;
            break;

        case 2:
            _channel_3_fp_power = front_pannel_power;
            _channel_3_actual_power = actual_power;
            _channel_3_max_power = maximum_power;
            break;

        case 3:
            _channel_4_fp_power = front_pannel_power;
            _channel_4_actual_power = actual_power;
            _channel_4_max_power = maximum_power;
            break;
    }
}

void CDisplay::set_current_menu(CMenu *menu)
{
    _current_menu = menu;
}

// Get the display area that's free for misc. content. I.e. the area that's not been taken by
// the top/bottom soft button text, and the bar graphs at the right
struct display_area CDisplay::get_display_area()
{
    struct display_area area;
    area.x0 = 0;
    area.y0 = menu_bar_height + 1;
    area.x1 = (DISPLAY_WIDTH-1) - (4*bar_width) - 1;
    area.y1 = (DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 1; 
    return area;
}

void CDisplay::set_battery_percentage(uint8_t bat)
{
    _battery_percentage = bat;
}

void CDisplay::set_active_pattern(std::string pattern)
{
    _active_pattern = pattern;
}

void CDisplay::draw_soft_buttons()
{
    color_t text_colour = hagl_color(0xFF, 0, 0);
    color_t line_colour = hagl_color(0xFF, 0, 0);

    // A
    put_text(_option_a, 3, (menu_bar_height/2)-6, text_colour);
    hagl_draw_rectangle(0, 0, (DISPLAY_WIDTH-1)/2, menu_bar_height, line_colour);
    
    // B
    put_text(_option_b, 3, (DISPLAY_HEIGHT-1) - (menu_bar_height/2)-6-status_bar_height, text_colour);
    hagl_draw_rectangle(0, (DISPLAY_HEIGHT-1)-menu_bar_height-status_bar_height, (DISPLAY_WIDTH-1)/2, (DISPLAY_HEIGHT-1)-status_bar_height, line_colour);

    // C
    put_text(_option_c, (DISPLAY_WIDTH/2)+3, (menu_bar_height/2)-6, text_colour);
    hagl_draw_rectangle((DISPLAY_WIDTH-1)/2, 0, (DISPLAY_WIDTH-1), menu_bar_height, line_colour);

    // D
    put_text(_option_d, (DISPLAY_WIDTH/2)+3, (DISPLAY_HEIGHT-1) - (menu_bar_height/2)-6-status_bar_height, text_colour);
    hagl_draw_rectangle((DISPLAY_WIDTH-1)/2, (DISPLAY_HEIGHT-1)-menu_bar_height-status_bar_height, (DISPLAY_WIDTH-1), (DISPLAY_HEIGHT-1)-status_bar_height, line_colour);
}

void CDisplay::draw_bar_graphs()
{  
    color_t normal_colour   =  hagl_color(0x00, 0x00, 0xFF);

    draw_bar(4, "1", _channel_1_max_power, _channel_1_fp_power, _channel_1_actual_power, normal_colour);
    draw_bar(3, "2", _channel_2_max_power, _channel_2_fp_power, _channel_2_actual_power, normal_colour);
    draw_bar(2, "3", _channel_3_max_power, _channel_3_fp_power, _channel_3_actual_power, normal_colour);
    draw_bar(1, "4", _channel_4_max_power, _channel_4_fp_power, _channel_4_actual_power, normal_colour);
}

void CDisplay::draw_bar(uint8_t bar_number, std::string label, uint16_t max_power, uint16_t front_pannel_power, uint16_t current_power, color_t bar_colour)
{
    put_text(label, (DISPLAY_WIDTH-1)-(bar_number*bar_width)+2, (DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 10, hagl_color(0, 0xFF, 0));
    hagl_draw_rectangle((DISPLAY_WIDTH-1)-(bar_number*bar_width), (DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height- 1, (DISPLAY_WIDTH-1)-((bar_number-1)*bar_width), menu_bar_height+1, hagl_color(0xFF, 0, 0));

    uint8_t upper_limit = (DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 2 - 10;
    uint8_t bottom = menu_bar_height + 2;

    // Max power outline
    float max_power_top = (((1000-max_power)/(float)1000) * ((float)upper_limit-(float)bottom)) + (float)bottom;
    hagl_fill_rectangle((DISPLAY_WIDTH-1)-(bar_number*bar_width)+1, (DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 2 - 10, (DISPLAY_WIDTH-1)-((bar_number-1)*bar_width)-1, max_power_top, bar_colour);

    // Front pannel power setting
    float fp_power_top = (((1000-front_pannel_power)/(float)1000) * ((float)upper_limit-(float)bottom)) + (float)bottom;
    float fp_power_bottom = fp_power_top-1;
    if (fp_power_bottom < 0)
        fp_power_bottom = 0;

    hagl_fill_rectangle(
            (DISPLAY_WIDTH-1)-(bar_number*bar_width)+1, 
            fp_power_bottom,
            (DISPLAY_WIDTH-1)-((bar_number-1)*bar_width)-1,
            fp_power_top, 
            bar_colour);

    // current power
    color_t current_power_colour   =  hagl_color(0xFF, 0xFF, 0x00);
    float current_power_top = (((1000-current_power)/(float)1000) * ((float)upper_limit-(float)bottom)) + (float)bottom;
    hagl_fill_rectangle((DISPLAY_WIDTH-1)-(bar_number*bar_width)+4, (DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 2 - 10, (DISPLAY_WIDTH-1)-((bar_number-1)*bar_width)-4, current_power_top, current_power_colour);
}

void CDisplay::draw_status_bar()
{
    char buffer[100] = {0};

    std::string current_mode = "N/A";
    if (_current_menu)
    {
        current_mode = _current_menu->get_title();
    }

    snprintf(buffer, sizeof(buffer), "BAT: %d%%  %s", _battery_percentage, current_mode.c_str());
    put_text(buffer, 0, (DISPLAY_HEIGHT-1) - status_bar_height+2, hagl_color(0xAA, 0xAA, 0xAA));
}

void CDisplay::put_text(std::string text, int16_t x, int16_t y, color_t color)
{
    if (text == "")
        text = " ";

    std::wstring widestr = std::wstring(text.begin(), text.end());
    hagl_put_text(widestr.c_str(), x, y, color, font6x9);
}

void CDisplay::set_update_required()
{
    _update_required = true;
}