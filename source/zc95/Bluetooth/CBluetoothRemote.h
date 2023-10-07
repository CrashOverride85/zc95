#ifndef _CBLUETOOTHREMOTE_H
#define _CBLUETOOTHREMOTE_H

#include <string>
#include <inttypes.h>

#define HID_USAGE_PAGE_GENERIC_DESKTOP  0x01
#define HID_USAGE_PAGE_BUTTON           0x09

// PAGE_GENERIC_DESKTOP
#define HID_USAGE_GENERIC_DESKTOP_X 0x30
#define HID_USAGE_GENERIC_DESKTOP_Y 0x31

// PAGE_BUTTON
#define HID_USAGE_BUTTON_NONE       0x00
#define HID_USAGE_BUTTON_PRIMARY    0x01

class CBluetoothRemote
{
    public:
        enum direction_t
        {
            DIR_UP,
            DIR_DOWN,
            DIR_LEFT,
            DIR_RIGHT
        };
        
        enum keypress_t
        {
            KEY_UP,
            KEY_DOWN,
            KEY_LEFT,
            KEY_RIGHT,
            KEY_OK,
            KEY_SHUTTER
        };

        CBluetoothRemote();
        ~CBluetoothRemote();

        void process_input(uint16_t usage_page, uint16_t usage, int32_t value);
        void end_of_input();

    private:
        int32_t _last_x;
        int32_t _last_y;
        int32_t _current_x;
        int32_t _current_y;
        direction_t _direction;
        bool _pressed;

        void report_keypress();
        void process_desktop_page(uint16_t usage, int32_t value);
        void process_button_page(uint16_t usage, int32_t value);
        
};

#endif