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
#include <font5x7.h>
#include <fontx.h>
#include <string>
#include <string.h>

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
    printf("CDisplay()\n");
    memset(_channel_power, 0, sizeof (_channel_power));

    _last_update = 0;
    _current_menu = NULL;

    fontx_glyph_t glyph;
    fontx_glyph(&glyph, L'A', font6x9);
    _rotate90_buffer = (uint8_t*)calloc(HAGL_CHAR_BUFFER_SIZE, sizeof(uint8_t));

    _font_width = glyph.width;
    _font_height = glyph.height;
    _battery_percentage = 0;
    _active_pattern = "";
    _remote_mode_active = false;
}

CDisplay::~CDisplay()
{
    printf("~CDisplay()\n");
    if (_rotate90_buffer)
    {
        free(_rotate90_buffer);
        _rotate90_buffer = NULL;
    }
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
    _hagl_backend = hagl_init();
    hagl_clear(_hagl_backend);
    update();
 }

 void CDisplay::update()
 {
    // Don't try to update the screen more than once every 100ms
    if (
        (_update_required || (time_us_64() - _last_update > (100*1000))) &&

        // It takes ~18ms for the display to finish refreshing (via DMA from hagl_flush()), so don't
        // start altering the back buffer if it's less than that, even if _update_required is set
        (time_us_64() - _last_update > (18*1000)) 
       )
    {
        _interuptable_section.start();
        
        _update_required = false;
        CTimingTest timing;
        hagl_clear(_hagl_backend);

        if (g_SavedSettings->power_level_show_disappearing_text() && _show_power_level_until > time_us_64())
        {
            draw_power_level();
        }
        else
        {
            if (_current_menu != NULL)
            {
                _current_menu->update();
            }

            draw_soft_buttons(); // 846us
        }

        draw_bar_graphs(); // 489us

        draw_status_bar();
  
        _interuptable_section.end();
        hagl_flush(_hagl_backend); // 8us, starts/uses DMA for update

        _last_update = time_us_64();
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
void CDisplay::set_power_level(uint8_t channel, int16_t front_panel_power, int16_t actual_power, int16_t maximum_power, bool remote_mode_active) // chan 0-3, others 0-1000
{
    if (front_panel_power > 1000)
        front_panel_power = 1000;
    if (front_panel_power < 0)
        front_panel_power = 0;

    if (actual_power > 1000)
        actual_power = 1000;
    if (actual_power < 0)
        actual_power = 0;

    if (maximum_power > 1000)
        maximum_power = 1000;
    if (maximum_power < 0)
        maximum_power = 0;

    if (channel >= MAX_CHANNELS)
    {
        printf("CDisplay::set_power_level - ERROR - invalid channel: %d\n", channel);
        return;
    }

    _remote_mode_active = remote_mode_active;

    if (front_panel_power != _channel_power[channel].fp_power)
    {      
        // Only show power level on screen if the integer percent value will actually be different.
        // Note: Display is 0-100, ADC is 8-bit (0-255) which is mapped to a "front_panel_power" of 
        //       0-1000, so small changes won't always change the on screen value
        if (_channel_power[channel].fp_power/10 != front_panel_power/10)
        {
            _show_power_level_until = time_us_64() + (1000 * 750); // show for 750ms
        }

        _channel_power[channel].fp_power = front_panel_power;
    }
    
    _channel_power[channel].actual_power = actual_power;
    _channel_power[channel].max_power = maximum_power;
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
    area.x1 = (MIPI_DISPLAY_WIDTH-1) - (4*bar_width) - 1;
    area.y1 = (MIPI_DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 1; 
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
    hagl_color_t text_colour = hagl_color(_hagl_backend, 0xFF, 0, 0);
    hagl_color_t line_colour = hagl_color(_hagl_backend, 0xFF, 0, 0);

    // A
    put_text(_option_a, 3, (menu_bar_height/2)-6, text_colour);
    hagl_draw_rectangle(_hagl_backend, 0, 0, (MIPI_DISPLAY_WIDTH-1)/2, menu_bar_height, line_colour);
    
    // B
    put_text(_option_b, 3, (MIPI_DISPLAY_HEIGHT-1) - (menu_bar_height/2)-6-status_bar_height, text_colour);
    hagl_draw_rectangle(_hagl_backend, 0, (MIPI_DISPLAY_HEIGHT-1)-menu_bar_height-status_bar_height, (MIPI_DISPLAY_WIDTH-1)/2, (MIPI_DISPLAY_HEIGHT-1)-status_bar_height, line_colour);

    // C
    put_text(_option_c, (MIPI_DISPLAY_WIDTH/2)+3, (menu_bar_height/2)-6, text_colour);
    hagl_draw_rectangle(_hagl_backend, (MIPI_DISPLAY_WIDTH-1)/2, 0, (MIPI_DISPLAY_WIDTH-1), menu_bar_height, line_colour);

    // D
    put_text(_option_d, (MIPI_DISPLAY_WIDTH/2)+3, (MIPI_DISPLAY_HEIGHT-1) - (menu_bar_height/2)-6-status_bar_height, text_colour);
    hagl_draw_rectangle(_hagl_backend, (MIPI_DISPLAY_WIDTH-1)/2, (MIPI_DISPLAY_HEIGHT-1)-menu_bar_height-status_bar_height, (MIPI_DISPLAY_WIDTH-1), (MIPI_DISPLAY_HEIGHT-1)-status_bar_height, line_colour);
}

void CDisplay::draw_bar_graphs()
{  
    hagl_color_t normal_colour   =  hagl_color(_hagl_backend, 0x00, 0x00, 0xFF);

    draw_bar(4, "1", _channel_power[0].max_power, _channel_power[0].fp_power, _channel_power[0].actual_power, normal_colour);
    draw_bar(3, "2", _channel_power[1].max_power, _channel_power[1].fp_power, _channel_power[1].actual_power, normal_colour);
    draw_bar(2, "3", _channel_power[2].max_power, _channel_power[2].fp_power, _channel_power[2].actual_power, normal_colour);
    draw_bar(1, "4", _channel_power[3].max_power, _channel_power[3].fp_power, _channel_power[3].actual_power, normal_colour);
}

void CDisplay::draw_bar(uint8_t bar_number, std::string label, uint16_t max_power, uint16_t front_panel_power, uint16_t current_power, hagl_color_t bar_colour)
{
    put_text(label, (MIPI_DISPLAY_WIDTH-1)-(bar_number*bar_width)+2, (MIPI_DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 10, hagl_color(_hagl_backend, 0, 0xFF, 0));
    hagl_draw_rectangle(_hagl_backend, (MIPI_DISPLAY_WIDTH-1)-(bar_number*bar_width), (MIPI_DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height- 1, (MIPI_DISPLAY_WIDTH-1)-((bar_number-1)*bar_width), menu_bar_height, hagl_color(_hagl_backend, 0xFF, 0, 0));

    // 0,0 is in the top left. Display area is the bit we can draw in (free of the top and bottom menu bars and the status bar at the bottom)
    uint8_t bottom_of_display_area = (MIPI_DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 2 - 10;
    uint8_t top_of_display_area = menu_bar_height + 2;

    // When in remote access mode, the front panel control are used to set the power limit, so mark the part of the power bar that can't be
    // reached due to the limit in green.
    if (_remote_mode_active)
    {
        float power_limit_bottom = (((1000-front_panel_power)/(float)1000) * ((float)bottom_of_display_area-(float)top_of_display_area)) + (float)top_of_display_area;
        hagl_fill_rectangle(_hagl_backend,
            (MIPI_DISPLAY_WIDTH-1)-(bar_number*bar_width)+2,      // x0
            top_of_display_area,                                  // y0
            (MIPI_DISPLAY_WIDTH-1)-((bar_number-1)*bar_width)-2,  // x1
            power_limit_bottom,                                   // y1
            hagl_color(_hagl_backend, 0x00, 0xFF, 0x00));
    }

    // Blue max power bar
    float max_power_top = (((1000-max_power)/(float)1000) * ((float)bottom_of_display_area-(float)top_of_display_area)) + (float)top_of_display_area;
    hagl_fill_rectangle(_hagl_backend, 
        (MIPI_DISPLAY_WIDTH-1)-(bar_number*bar_width)+1, 
        (MIPI_DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 2 - 10, 
        (MIPI_DISPLAY_WIDTH-1)-((bar_number-1)*bar_width)-1, 
        max_power_top, 
        bar_colour);
        
    // Front panel power setting
    float fp_power_top = (((1000-front_panel_power)/(float)1000) * ((float)bottom_of_display_area-(float)top_of_display_area)) + (float)top_of_display_area;
    float fp_power_bottom = fp_power_top-1;
    if (fp_power_bottom < 0)
        fp_power_bottom = 0;

    // Blue max power line (main blue bar ramps up to this)
    hagl_fill_rectangle(_hagl_backend, 
            (MIPI_DISPLAY_WIDTH-1)-(bar_number*bar_width)+1, 
            fp_power_bottom,
            (MIPI_DISPLAY_WIDTH-1)-((bar_number-1)*bar_width)-1,
            fp_power_top, 
            bar_colour);

    // current power
    hagl_color_t current_power_colour   =  hagl_color(_hagl_backend, 0xFF, 0xFF, 0x00);
    float current_power_top = (((1000-current_power)/(float)1000) * ((float)bottom_of_display_area-(float)top_of_display_area)) + (float)top_of_display_area;
    hagl_fill_rectangle(_hagl_backend, (MIPI_DISPLAY_WIDTH-1)-(bar_number*bar_width)+4, (MIPI_DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 2 - 10, (MIPI_DISPLAY_WIDTH-1)-((bar_number-1)*bar_width)-4, current_power_top, current_power_colour);

    if (g_SavedSettings->power_level_show_in_bar_graph())
        put_text(std::to_string(front_panel_power/10), (MIPI_DISPLAY_WIDTH-1)-(bar_number*bar_width)+2, (MIPI_DISPLAY_HEIGHT-1) - menu_bar_height - status_bar_height - 30, hagl_color(_hagl_backend, 0, 0xFF, 0), true);
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
    put_text(buffer, 0, (MIPI_DISPLAY_HEIGHT-1) - status_bar_height+2, hagl_color(_hagl_backend, 0xAA, 0xAA, 0xAA));
}

void CDisplay::draw_power_level()
{
    const uint8_t scale_factor = 3;

    fontx_glyph_t glyph;
    fontx_glyph(&glyph, L'0', font6x9);

    char buffer[6] = {0};
    hagl_bitmap_t bitmap;
    hagl_color_t tc = hagl_color(_hagl_backend, 0xAA, 0xAA, 0xAA);

    for (uint8_t j=0; j<MAX_CHANNELS; j++) 
    {
        snprintf(buffer, sizeof(buffer), "%3d", _channel_power[j].fp_power/10);
        bitmap.buffer = (uint8_t*)calloc(HAGL_CHAR_BUFFER_SIZE, sizeof(uint8_t));

        for (uint8_t i=0; i<3; i++) 
        {
            uint8_t scaled_glyph_width  = (glyph.width  * scale_factor);
            uint8_t scaled_glyph_height = (glyph.height * scale_factor);

            hagl_get_glyph(_hagl_backend, buffer[i], tc, &bitmap, font6x9);
            hagl_blit_xywh(_hagl_backend, i*scaled_glyph_width, j*scaled_glyph_height, scaled_glyph_width, scaled_glyph_height, &bitmap);
        }

        free(bitmap.buffer);
    }
}

void CDisplay::put_text(std::string text, int16_t x, int16_t y, hagl_color_t color, bool rotate90)
{
    if (text == "")
        text = " ";

    std::wstring widestr = std::wstring(text.begin(), text.end());
    if (rotate90)
        hagl_put_text_rotate90(_hagl_backend, widestr.c_str(), x, y, color, font5x7);
    else
        hagl_put_text(_hagl_backend, widestr.c_str(), x, y, color, font6x9);
}

void CDisplay::set_update_required()
{
    _update_required = true;
}

// Mostly copied from the hagl lib's hagl_put_char.
// Show a character rotated 90 degrees
uint8_t CDisplay::hagl_put_char_rotate90(void const *_surface, wchar_t code, int16_t x0, int16_t y0, hagl_color_t color, const uint8_t *font)
{
    const hagl_surface_t *surface = (hagl_surface_t*)_surface;
    uint8_t set, status;
    hagl_bitmap_t bitmap;
    fontx_glyph_t glyph;

    status = fontx_glyph(&glyph, code, font);

    if (0 != status) 
    {
        return 0;
    }

    hagl_bitmap_init(&bitmap,  glyph.height, glyph.width, surface->depth, (uint8_t *)_rotate90_buffer);
    hagl_color_t *ptr = (hagl_color_t *) bitmap.buffer;


    for (uint8_t y = 0; y < glyph.height; y++) {
        for (uint8_t x = 0; x < glyph.width; x++) {
    
            set = *(glyph.buffer + x / 8) & (0x80 >> (x % 8));
            uint8_t idx = glyph.height*(glyph.width-1-x) + y;

            (ptr)[idx] = set ? color : 0x0000;
        }
        glyph.buffer += glyph.pitch;
    }

    hagl_blit(surface, x0, y0, &bitmap);

    return bitmap.height;
}

// Mostly copied from the hagl lib's hagl_put_text
// Show a string rotated 90 degrees
void CDisplay::hagl_put_text_rotate90(void const *surface, const wchar_t *str, int16_t x0, int16_t y0, hagl_color_t color, const unsigned char *font)
{
    wchar_t temp;
    uint8_t status;
    fontx_meta_t meta;

    status = fontx_meta(&meta, font);
    if (0 != status)
        return;

    do 
    {
        temp = *str++;
        y0 -= hagl_put_char_rotate90(surface, temp, x0, y0, color, font);
    } while (*str != 0);

    return;
}

hagl_backend_t* CDisplay::get_hagl_backed()
{
    return _hagl_backend;
}

