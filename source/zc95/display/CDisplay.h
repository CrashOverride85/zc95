#ifndef _CDISPLAY_H
#define _CDISPLAY_H


#include <string>

#include <hagl_hal.h>
#include <hagl.h>

#include <fps.h>
#include <aps.h>

#include "CMenu.h"
#include "../CUtil.h"
#include "../config.h"
#include "../FrontPanel/CFrontPanel.h"
#include "../Bluetooth/CBluetooth.h"

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
        CDisplay(CFrontPanel *front_panel, CBluetooth *bluetooth);
        ~CDisplay();
        void init();
        void update();
        void set_option_a(std::string text);
        void set_option_b(std::string text);
        void set_option_c(std::string text);
        void set_option_d(std::string text);
        
        void set_power_level(uint8_t channel, int16_t front_panel_power, int16_t actual_power, int16_t maximum_power, bool remote_mode_active);

        void set_current_menu(CMenu *menu);

        struct display_area get_display_area();
        void put_text(std::string text, int16_t x, int16_t y, hagl_color_t color, bool rotate90 = false, const uint8_t *font = NULL);
        uint8_t get_font_width();
        uint8_t get_font_height();
        void set_battery_percentage(uint8_t bat);
        uint8_t get_battery_percentage();
        void set_active_pattern(std::string pattern);
        void set_update_required();
        hagl_backend_t* get_hagl_backed();

    private:
        struct power_levels_t
        {
            // All power levels are 0 - 1000
            int16_t fp_power;      // Front panel power - what the front panel is set to
            int16_t max_power;     // Curent maximum power for the channel (displayed as blue bar)
            int16_t actual_power;  // Power level currently being output. After inital ramp up, usually the same as max_power (but routines can reduce it)
        };

        void draw_soft_buttons();
        void draw_status_bar();
        void draw_power_level();
        void draw_bar_graphs();
        void draw_bar(uint8_t bar_number, std::string label, uint16_t max_power, uint16_t front_panel_power, uint16_t current_power, hagl_color_t bar_colour);
        uint8_t hagl_put_char_rotate90(void const *_surface, wchar_t code, int16_t x0, int16_t y0, hagl_color_t color, const uint8_t *font);
        void hagl_put_text_rotate90(void const *surface, const wchar_t *str, int16_t x0, int16_t y0, hagl_color_t color, const unsigned char *font);
        void draw_logo(const uint8_t logo[9], int16_t x0, int16_t y0, hagl_color_t colour);
        void draw_bt_logo_if_required(int16_t x, int16_t y);
        void draw_battery_icon(int16_t x, int16_t y);
        void draw_power_level_indicator(int16_t x, int16_t y);

        // Soft buttons
        std::string _option_a; // top left
        std::string _option_b; // bottom left
        std::string _option_c; // top right
        std::string _option_d; // bottom right

        power_levels_t _channel_power[MAX_CHANNELS]; // Note that _channel_power[0] is channel 1, _channel_power[1] is channel 2, etc.

        uint64_t _last_update;

        CMenu *_current_menu;

        uint8_t _font_width;
        uint8_t _font_height;

        uint8_t _battery_percentage;
        std::string _active_pattern;
        bool _update_required;
        CInteruptableSection _interruptable_section;
        bool _remote_mode_active;
        hagl_backend_t *_hagl_backend = NULL;
        uint8_t *_rotate90_buffer = NULL;
        uint64_t _show_power_level_until = 0;
        CFrontPanel *_front_panel;
        CBluetooth *_bluetooth;
};

#endif
