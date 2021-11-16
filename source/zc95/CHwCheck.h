
#ifndef _CHWCHECK_H
#define _CHWCHECK_H

#include <list>
#include <string>
#include <inttypes.h>

#include <hagl_hal.h>
#include <hagl.h>

#include "hardware/i2c.h"


class CHwCheck
{
    public:
        CHwCheck();
        void check();
        uint8_t get_battery_percentage();

    private:
        enum Cause {UNKNOWN, MISSING, BATTERY};
        void show_error_text_missing(int y);
        void show_error_text_message(int y, std::string message);
        void hw_check_failed(enum Cause casue);
        void put_text(std::string text, int16_t x, int16_t y, color_t color);
        static int cmpfunc (const void *a, const void *b);
        float get_adc_voltage();
        float get_battery_voltage(float adc_voltage);

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

        std::list<device> _devices;
};

#endif
