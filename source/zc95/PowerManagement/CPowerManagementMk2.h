
#ifndef _CCPOWERMANAGEMENTMK2_H
#define _CCPOWERMANAGEMENTMK2_H

#include <list>
#include <string>
#include <inttypes.h>
#include "IPowerManagement.h"
#include "bq27441.h"

#include "globals.h"
#include "CUtil.h"
#include "CUsbPower.h"
#include "../PortExpanders/CMainBoardPortExp.h"

class CPowerManagementMk2 : public IPowerManagement
{
    public:
        CPowerManagementMk2(CMainBoardPortExp* mainboard_port_exp, hw_variant_t variant);
        void print_status();

        // for IPowerManagement
        void loop();
        power_status_t power_status();
        charging_status_t charging_status();
        bool get_stat(int16_t* stat, power_stat_t type);
        uint8_t get_battery_percentage();

        void add_raw_adc_readings(const uint8_t *raw_adc_readings_buffer, uint8_t buffer_array_len);
        void set_inital_cc_voltages_and_set_input_current_limit(int16_t cc1_mv, int16_t cc2_mv);

    private:
        static int s_cmpfunc (const void *a, const void *b);
        static int s_cmpfunc_uint8_t (const void *a, const void *b);
        static int16_t s_BQ27441_i2cWriteBytes(uint8_t DevAddress, uint8_t subAddress, uint8_t* src, uint8_t count);
        static int16_t s_BQ27441_i2cReadBytes(uint8_t DevAddress, uint8_t subAddress, uint8_t* dest, uint8_t count);

        void set_adc0_source(CMainBoardPortExp::adc0_select_t adc_source);
        void loop_v2_0();
        void loop_v2_2();

        CMainBoardPortExp* _mainboard_port_exp = NULL;
        hw_variant_t _variant;
        BQ27441_ctx_t _BQ27441;
        bool _BQ27441_init_success = false;

        CUsbPower _usb_power;
        power_status_t _power_status = power_status_t::Unknown;
        charging_status_t _charging_status = charging_status_t::Unknown;
        
        uint8_t _battery_percentage = 0xFF;
        int16_t _battery_voltage = -1;
        int16_t _current_mA;
        uint16_t _remaining_capacity_mah;
        uint16_t _full_capacity_mah;
        int16_t _vbus_voltage = -1;

        uint16_t _input_limit = 0;

        uint64_t _last_batt_param_refresh = 0;
        uint64_t _last_adc0_read;
        CMainBoardPortExp::adc0_select_t _adc0_source;
        bool _inital_startup = true;
};

#endif


