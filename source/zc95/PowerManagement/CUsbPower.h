
#ifndef _CUSBPOWER_H
#define _CUSBPOWER_H

#include <list>
#include <string>
#include <inttypes.h>
#include "IPowerManagement.h"
#include "BQ25601.h"
#include "../ZcTypes.h"

class CUsbPower 
{
    public:
        CUsbPower(hw_variant_t variant);
        void set_cc1_voltage_mV(int16_t mv);
        void set_cc2_voltage_mV(int16_t mv);
        bool ext_power_good();
        BQ25601::charge_status_enum charge_status();
        void loop();

    private:
        enum class usb_power_t
        {
            UNKNOWN,
            CURRENT_1A,
            CURRENT_1_5A,
            CURRENT_3A
        };

        usb_power_t get_usb_power();
        std::string usb_power_status_string(usb_power_t usb_power_status);
        void update_usb_power_status();
        void update_input_current_limit();
        uint16_t get_current_limit_ma(usb_power_t usb);
        void set_charge_current();
        constexpr time_us_t SecondsInUs(int seconds) { return (seconds * 1000000); }
        
        bool _active = false;
        uint16_t _reported_current_limit_ma = 0;
        bool _pg_good = false;
        int16_t _cc1_voltage = -1;
        int16_t _cc2_voltage = -1;
        BQ25601 _charge_ctl;
        usb_power_t _usb_power = usb_power_t::UNKNOWN;
        time_us_t _time_usb_power_changed = 0;
};

#endif
