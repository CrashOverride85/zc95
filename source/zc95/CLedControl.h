#ifndef _CLEDCONTROL_H
#define _CLEDCONTROL_H

#include <inttypes.h>
#include "hardware/pio.h"

#include "CSavedSettings.h"

 enum class LED: uint8_t
 {
     Trigger1 = 0,
     Trigger2 = 1,
     Channel1 = 2,
     Channel2 = 3,
     Channel3 = 4,
     Channel4 = 5
 };
#define LED_COUNT    6

enum LedColour: uint32_t
{
    Black  = 0,
    White  = 16777215,
    Red    = 16711680,
    Lime   = 65280,
    Blue   = 255,
    Yellow = 16776960,
    Cyan   = 65535,
    Magenta= 16711935,
    Silver = 12632256,
    Gray   = 8421504,
    Maroon = 8388608,
    Olive  = 8421376,
    Green  = 32768,
    Purple = 8388736,
    Teal   = 32896,
    Navy   = 128
};


class CLedControl
{
    public:
        CLedControl(uint8_t tx_pin, CSavedSettings *settings);
        ~CLedControl();
        void loop(bool force_update = false);
        void update_leds();
        void init();
        void set_led_colour(LED led, uint32_t colour);
        uint32_t get_brightness_adjusted_led_colour(uint8_t led);
        void set_all_led_colour(uint32_t colour);
    
    private:
        void put_pixel(uint32_t pixel_rgb);
        uint8_t get_led_brightness();
        volatile uint32_t _led_state[LED_COUNT];
        volatile bool _led_state_changed;
        uint8_t _tx_pin;
        PIO _pio;
        int _sm;
        int8_t _brightness;
        CSavedSettings *_settings;
        uint64_t _last_led_update = 0;
};

#endif