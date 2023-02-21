#ifndef _CDISPLAY_H
#define _CDISPLAY_H


#include <string>

#include <hagl_hal.h>
#include <hagl.h>

#include <fps.h>
#include <aps.h>

#include "CMenu.h"
#include "../CUtil.h"

struct display_area
{
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
};

class CDisplay
{
    public:
        CDisplay();
        void init();
        void update();
        void set_option_a(std::string text);
        void set_option_b(std::string text);
        void set_option_c(std::string text);
        void set_option_d(std::string text);
        
        void set_power_level(uint8_t channel, int16_t front_pannel_power, int16_t actual_power, int16_t maximum_power, bool remote_mode_active);

        void set_current_menu(CMenu *menu);

        struct display_area get_display_area();
        void put_text(std::string text, int16_t x, int16_t y, color_t color);
        uint8_t get_font_width();
        uint8_t get_font_height();
        void set_battery_percentage(uint8_t bat);
        void set_active_pattern(std::string pattern);
        void set_update_required();

    private:
        void draw_soft_buttons();
        void draw_status_bar();
        void draw_bar_graphs();
        void draw_bar(uint8_t bar_number, std::string label, uint16_t max_power, uint16_t front_pannel_power, uint16_t current_power, color_t bar_colour);
        
        // Soft buttons
        std::string _option_a; // top left
        std::string _option_b; // bottom left
        std::string _option_c; // top right
        std::string _option_d; // bottom right

        // Front Pannel power levels, all 0-1000
        int16_t _channel_1_fp_power; 
        int16_t _channel_2_fp_power; 
        int16_t _channel_3_fp_power; 
        int16_t _channel_4_fp_power; 

        // Current maximum power level, 0-1000
        int16_t _channel_1_max_power; 
        int16_t _channel_2_max_power; 
        int16_t _channel_3_max_power; 
        int16_t _channel_4_max_power; 

        int16_t _channel_1_actual_power = 0; 
        int16_t _channel_2_actual_power = 0;
        int16_t _channel_3_actual_power = 0;
        int16_t _channel_4_actual_power = 0;

        int16_t _channel_1_remote_power = 0; 
        int16_t _channel_2_remote_power = 0; 
        int16_t _channel_3_remote_power = 0; 
        int16_t _channel_4_remote_power = 0; 

        uint64_t _last_update;

        CMenu *_current_menu;

        uint8_t _font_width;
        uint8_t _font_height;

        uint8_t _battery_percentage;
        std::string _active_pattern;
        bool _update_required;
        CInteruptableSection _interuptable_section;
        bool _remote_mode_active;
};

#endif
