
#ifndef _CHWCHECK_H
#define _CHWCHECK_H

#include <list>
#include <string>
#include <inttypes.h>

#include <hagl_hal.h>
#include <hagl.h>

#include "hardware/i2c.h"
#include "CLedControl.h"
#include "CMainBoardPortExp.h"
#include "CBatteryGauge.h"
#include "display/CDisplay.h"
#include "core1/output/ZC624Output/CZC624Comms.h"

#define BAT_AVG_COUNT 50

class CHwCheck
{
    public:
        enum class front_panel_version_t
        {
            UNKNOWN,
            v0_1,
            v0_2
        };

        CHwCheck(CBatteryGauge *batteryGauge);
        ~CHwCheck();
        void check_part1();
        void check_part2(CLedControl *ledControl, CMainBoardPortExp *controls);
        void process();
        std::string get_zc624_version();
        bool audio_digipot_found();
        void die(CLedControl *led_control, std::string error_message);
        bool clear_eeprom_buttons_pressed();
        void clear_eeprom_if_requested();
        static bool running_on_picow();
        void set_display(CDisplay *display);    
        CHwCheck::front_panel_version_t get_front_panel_version();

    private:
        enum Cause {UNKNOWN, MISSING, BATTERY, ZC624_UNKNOWN, ZC624_STATUS, ZC624_VERSION, ZC624_NO_SPI, NO_FP_ADC};
        void show_error_text_missing(int y);
        void show_error_text_message(int *y, std::string message);
        void hw_check_failed(enum Cause casue, CLedControl *ledControl, CMainBoardPortExp *controls);
        void put_text(std::string text, int16_t x, int16_t y, hagl_color_t color);
        void get_battery_readings();
        void halt(CLedControl *led_control);
        void report_zc624_fault(int *y);
        front_panel_version_t determine_front_panel_version();
        void set_expected_devices(front_panel_version_t ver);
        void init_v0_2_front_panel();
        int  get_button_press();

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
        hagl_backend_t *_hagl_backend = NULL;
        uint8_t _zc624_status = 0;
        front_panel_version_t _front_panel_version = front_panel_version_t::UNKNOWN;
};

#endif
