#ifndef _CBLUETOOTHREMOTE_H
#define _CBLUETOOTHREMOTE_H

#include <string>
#include <inttypes.h>

// I'm not sure where these are supposed to come from for bluetooth, but these are from:
// HID Usage Tables
// FOR
// Universal Serial Bus (USB)
// Version 1.4

// MC = Momentary Control: a basic push button (value expected to be 1 or 0)
// DV = Dynamic Value: "A Dynamic Value is an n-bit field that contains a value associated with a control"

#define HID_USAGE_PAGE_GENERIC_DESKTOP  0x01
#define HID_USAGE_PAGE_BUTTON           0x09
#define HID_USAGE_PAGE_DIGITIZER        0x0D

// PAGE_GENERIC_DESKTOP
#define HID_USAGE_GENERIC_DESKTOP_X 0x30
#define HID_USAGE_GENERIC_DESKTOP_Y 0x31

// PAGE_BUTTON
#define HID_USAGE_BUTTON_NONE       0x00
#define HID_USAGE_BUTTON_PRIMARY    0x01

// PAGE_DIGITIZER 
#define HID_USAGE_DIGITIZER_IN_RANGE            0x32 // (MC)
#define HID_USAGE_DIGITIZER_TIP_SWITCH          0x42 // (MC)
// For the next two, see HUTRR34, MultiTouch Digitizers, 2009, https://www.usb.org/sites/default/files/hutrr34.pdf
#define HID_USAGE_DIGITIZER_CONTACT_IDENTIFIER  0x51 // (DV) "An identifier associated with a contact."
#define HID_USAGE_DIGITIZER_CONTACT_COUNT       0x54 // (DV) "The current number of contacts the digitizer detects and is reporting."


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
        void process_digitizer_page(uint16_t usage, int32_t value);        
};

#endif