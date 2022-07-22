
#ifndef _CHWCHECK_H
#define _CHWCHECK_H

#include <list>
#include <string>
#include <inttypes.h>

#include <hagl_hal.h>
#include <hagl.h>

#include "hardware/i2c.h"
#include "CHwCheckZc624.h"
#include "CLedControl.h"
#include "CControlsPortExp.h"
#include "CBatteryGauge.h"

#define BAT_AVG_COUNT 50

class CHwCheck
{
    public:
        CHwCheck(CBatteryGauge *batteryGauge);
        void check_part1();
        void check_part2(CLedControl *ledControl, CControlsPortExp *controls);
        void process();
        std::string get_zc624_version();

    private:
        enum Cause {UNKNOWN, MISSING, BATTERY, ZC628};
        void show_error_text_missing(int y);
        void show_error_text_message(int y, std::string message);
        void hw_check_failed(enum Cause casue, CLedControl *ledControl, CControlsPortExp *controls);
        void put_text(std::string text, int16_t x, int16_t y, color_t color);
        void get_battery_readings();

        class device
        {
            public:
                uint8_t address;
                std::string description;
                std::string display;
                bool present;

                device(uint8_t addr, std::string desc, std::string disp)
                {
                    address = addr;
                    description = desc;
                    present = false;
                    display = disp;
                }
        };

        CHwCheckZc624 _checkZc624;
        std::list<device> _devices;
        CBatteryGauge *_batteryGauge;
};

#endif
