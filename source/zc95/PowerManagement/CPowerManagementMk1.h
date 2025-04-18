
#ifndef _CBATTGAUGE_H
#define _CBATTGAUGE_H

#include <list>
#include <string>
#include <inttypes.h>
#include "IPowerManagement.h"

#define BAT_AVG_COUNT 50

class CPowerManagementMk1 : public IPowerManagement
{
    public:
        CPowerManagementMk1();
        void add_raw_adc_readings(const uint8_t *raw_adc_readings_buffer, uint8_t buffer_array_len);
        void get_battery_readings();

        // for IPowerManagement
        void loop();
        power_status_t power_status();
        charging_status_t charging_status();
        bool get_stat(int16_t* stat, power_stat_t type);
        uint8_t get_battery_percentage();

    private:
        static int cmpfunc (const void *a, const void *b);
        static int cmpfunc_uint8_t (const void *a, const void *b);
        
        float get_adc_voltage();
        uint8_t get_battery_percentage_from_voltage(float battery_voltage);
        float get_battery_voltage_from_adc_reading(uint32_t adc_reading);        
  
        uint64_t _last_update;
        uint8_t _batt_percentage[BAT_AVG_COUNT] = {0};
        uint8_t _batt_reading_idx = 0;
        float _battery_voltage = 0;
        bool _inital_startup = true;
};

#endif
