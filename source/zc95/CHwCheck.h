
#ifndef _CHWCHECK_H
#define _CHWCHECK_H

#include <list>
#include <string>
#include <inttypes.h>

#include <hagl_hal.h>
#include <hagl.h>

#include "hardware/i2c.h"
#include "CLedControl.h"
#include "CControlsPortExp.h"
#include "CBatteryGauge.h"
#include "core1/output/ZC624Output/CZC624Comms.h"

#define BAT_AVG_COUNT 50

class CHwCheck
{
    public:
        CHwCheck(CBatteryGauge *batteryGauge);
        ~CHwCheck();
        void check_part1();
        void check_part2(CLedControl *ledControl, CControlsPortExp *controls);
        void process();
        std::string get_zc624_version();
        bool audio_digipot_found();
        void die(CLedControl *led_control, std::string error_message);
        static bool running_on_picow();

    private:
        enum Cause {UNKNOWN, MISSING, BATTERY, ZC624_UNKNOWN, ZC624_STATUS, ZC624_VERSION};
        void show_error_text_missing(int y);
        void show_error_text_message(int y, std::string message);
        void hw_check_failed(enum Cause casue, CLedControl *ledControl, CControlsPortExp *controls);
        void put_text(std::string text, int16_t x, int16_t y, color_t color);
        void get_battery_readings();
        void halt(CLedControl *led_control);

        class device
        {
            public:
                uint8_t address;
                std::string description;
                std::string display;
                bool optional;
                bool present;
                
                device(uint8_t addr, std::string desc, std::string disp, bool opt=false)
                {
                    address = addr;
                    description = desc;
                    present = false;
                    display = disp;
                    optional = opt;
                }
        };

        CZC624Comms *_zc624_comms;
        std::list<device> _devices;
        CBatteryGauge *_batteryGauge;
};

#endif
