
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

#define BAT_AVG_COUNT 50

class CHwCheck
{
    public:
        CHwCheck();
        void check_part1();
        void check_part2(CLedControl *ledControl, CControlsPortExp *controls);
        void process();
        uint8_t get_battery_percentage();
        std::string get_zc624_version();

    private:
        enum Cause {UNKNOWN, MISSING, BATTERY, ZC628};
        void show_error_text_missing(int y);
        void show_error_text_message(int y, std::string message);
        void hw_check_failed(enum Cause casue, CLedControl *ledControl, CControlsPortExp *controls);
        void put_text(std::string text, int16_t x, int16_t y, color_t color);
        static int cmpfunc (const void *a, const void *b);
        static int cmpfunc_uint8_t (const void *a, const void *b);
        
        float get_adc_voltage();
        float get_battery_voltage();
        uint8_t get_real_time_battery_percentage();

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
        uint64_t _last_update;
        uint8_t _batt_percentage[BAT_AVG_COUNT] = {0};
        uint8_t _batt_reading_idx = 0;
        bool _inital_startup = true;
};

#endif
