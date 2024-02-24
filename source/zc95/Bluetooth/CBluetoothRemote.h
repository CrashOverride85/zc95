#ifndef _CBLUETOOTHREMOTE_H
#define _CBLUETOOTHREMOTE_H

#include <string>
#include <inttypes.h>
#include "pico/util/queue.h"

// I'm not sure where these are supposed to come from for bluetooth, but these are from:
// HID Usage Tables
// FOR
// Universal Serial Bus (USB)
// Version 1.4

// MC = Momentary Control: a basic push button (value expected to be 1 or 0)
// DV = Dynamic Value: "A Dynamic Value is an n-bit field that contains a value associated with a control"
// LC = Linear Control: "In many cases, a control of a linear value is implemented as a pair of increment/decrement buttons, a jog wheel, or a linear control such as a knob or a slide"

#define HID_USAGE_PAGE_GENERIC_DESKTOP  0x01
#define HID_USAGE_PAGE_BUTTON           0x09
#define HID_USAGE_PAGE_CONSUMER         0x0C
#define HID_USAGE_PAGE_DIGITIZER        0x0D

// PAGE_GENERIC_DESKTOP
#define HID_USAGE_GENERIC_DESKTOP_X     0x30
#define HID_USAGE_GENERIC_DESKTOP_Y     0x31
#define HID_USAGE_GENERIC_DESKTOP_WHEEL 0x38 // Why would a BT button use this!? but some do! (and always pass the value as 0)

// PAGE_BUTTON
#define HID_USAGE_BUTTON_NONE           0x00
#define HID_USAGE_BUTTON_PRIMARY        0x01

// PAGE_DIGITIZER 
#define HID_USAGE_DIGITIZER_IN_RANGE            0x32 // (MC)
#define HID_USAGE_DIGITIZER_TIP_SWITCH          0x42 // (MC)
// For the next two, see HUTRR34, MultiTouch Digitizers, 2009, https://www.usb.org/sites/default/files/hutrr34.pdf
#define HID_USAGE_DIGITIZER_CONTACT_IDENTIFIER  0x51 // (DV) "An identifier associated with a contact."
#define HID_USAGE_DIGITIZER_CONTACT_COUNT       0x54 // (DV) "The current number of contacts the digitizer detects and is reporting."

// PAGE_CONSUMER
// I've seen phone/shutter remotes that send all of these, and some point I just gave up commenting them.
// Most are always sent with 0 value, so seem pretty pointless
#define HID_USAGE_CONSUMER_UNDEFINED        0x00  // Why would you ever send this!
#define HID_USAGE_CONSUMER_POWER            0x30  // "Controls the application-specific power state"
#define HID_USAGE_CONSUMER_SCAN_PREV_TRACK  0xB6
#define HID_USAGE_CONSUMER_EJECT            0xB8
#define HID_USAGE_CONSUMER_PLAY_PAUSE       0xCD
#define HID_USAGE_CONSUMER_VOL_INC          0xE9  // Common shutter button, as the camera app in iOS will take a photo when getting vol up or vol down
#define HID_USAGE_CONSUMER_VOL_DEC          0xEA 
#define HID_USAGE_CONSUMER_MUTE             0xE2 
#define HID_USAGE_CONSUMER_AL_KEY_LAYOUT    0x1AE // "Launch Keyboard Layout Management application"
#define HID_USAGE_CONSUMER_AL_SCREEN_SAVER  0x1B1 
#define HID_USAGE_CONSUMER_AC_SEARCH        0x221
#define HID_USAGE_CONSUMER_AC_HOME          0x223
#define HID_USAGE_CONSUMER_AC_PAN           0x238 // (LC) "Set the horizontal offset of the display in the document."     

class CBluetoothRemote
{
    public:       
        enum keypress_t
        {
            KEY_BUTTON,
            KEY_UP,
            KEY_DOWN,
            KEY_LEFT,
            KEY_RIGHT,
            KEY_SHUTTER,
            KEY_UNKNOWN
        };

        struct bt_keypress_queue_entry_t
        {
            CBluetoothRemote::keypress_t key;
        };

        CBluetoothRemote();
        ~CBluetoothRemote();

        void set_keypress_queue(queue_t *bt_keypress_queue);

        void process_input(uint16_t usage_page, uint16_t usage, int32_t value);
        static std::string s_get_keypress_string(keypress_t key);

    private:       
        struct dimension_t
        {
            uint8_t received_count; // how many received since last movement started event
            int32_t prev_val;       // 2nd most recent value received
            int32_t most_recent;    // most recent value received
        };
        
        queue_t *_bt_keypress_queue = NULL;
        bool _movement_started;
        dimension_t _x;
        dimension_t _y;
        uint64_t _last_button_event = 0;


        void process_desktop_page(uint16_t usage, int32_t value);
        bool is_movement_start(uint16_t usage_page, uint16_t usage, int32_t value);
        bool is_movement_end  (uint16_t usage_page, uint16_t usage, int32_t value);
        bool is_movement_event(uint16_t usage_page, uint16_t usage);
        bool is_shutter_button(uint16_t usage_page, uint16_t usage, int32_t value);
        
        void reset_dimension(dimension_t &dimension);

        keypress_t get_last_direction_button_pressed();
        void send_keypress(keypress_t key);

        // debug output functions
        void print_desktop_page(uint16_t usage, int32_t value);
        void print_button_page(uint16_t usage, int32_t value);
        void print_digitizer_page(uint16_t usage, int32_t value);
        void print_consumer_page(uint16_t usage, int32_t value);
};

#endif
