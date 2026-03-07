
#include "CPowerManagementMk1.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"

CPowerManagementMk1::CPowerManagementMk1()
{

}

void CPowerManagementMk1::get_battery_readings()
{
    uint8_t readings[10];

    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    // get 10 readings
    for (uint8_t reading_count=0; reading_count < sizeof(readings); reading_count++)
    {
        uint16_t reading = adc_read();
        readings[reading_count] = reading >> 4; // Convert 12bit ADC reading to 8bit (later reads via DMA are 8bit, so need to be consistant)
    }

    add_raw_adc_readings(readings, sizeof(readings));
}

void CPowerManagementMk1::add_raw_adc_readings(const uint8_t *raw_adc_readings_buffer, uint8_t buffer_array_len)
{
    uint32_t readings[10];
    uint8_t readings_len = sizeof(readings) / sizeof(readings[0]);

    // On initial startup all the readings in _batt_percentage will be 0, so try and set them to something better
    // as quick as possible.
    if (_inital_startup || (time_us_64() - _last_update > 1000000)) // 1sec
    {
        if (buffer_array_len < readings_len)
        {
            printf("CPowerManagementMk1::add_raw_adc_readings: need at least %d readings\n", readings_len);
            return;
        }
        
        // qsort sorts an array, but we don't want to alter the raw_adc_readings_buffer input array, so copy.
        for (uint8_t x=0; x < readings_len; x++)
            readings[x] = raw_adc_readings_buffer[x];

        // ignore 2 highest and 2 lowest values. get the average of the rest
        qsort(readings, readings_len, sizeof(uint32_t), CPowerManagementMk1::cmpfunc);
        uint32_t total=0;
        for (uint8_t reading=2; reading < readings_len-2; reading++)
            total += readings[reading];

        uint32_t avg = total/(readings_len-4);
        
        // Convert to voltage
        _battery_voltage = get_battery_voltage_from_adc_reading(avg);

       // Add to _batt_percentage
        _batt_reading_idx++;
        if (_batt_reading_idx >= BAT_AVG_COUNT)
        {
            _batt_reading_idx = 0;
            _inital_startup = false;
        }

        _batt_percentage[_batt_reading_idx] = get_battery_percentage_from_voltage(_battery_voltage);

        _last_update = time_us_64();
    }
}

float CPowerManagementMk1::get_battery_voltage_from_adc_reading(uint32_t adc_reading)
{
    const float conversion_factor = 3.3f / (1 << 8);

    float adc_voltage = adc_reading * conversion_factor;

    float r1 = 27000;      // R13 on mainboard
    float r2 = 4700;       // R15 on mainboard
    float i = adc_voltage / r2;
    float batt_voltage = i * (r1 + r2);

    //printf("adc_reading=%lu, adc_voltage=%fv, batt_voltage=%fv\n", adc_reading, adc_voltage, batt_voltage);

    return batt_voltage;
}

int CPowerManagementMk1::cmpfunc (const void *a, const void *b)
{
   return ( *(uint32_t*)a - *(uint32_t*)b );
}

int CPowerManagementMk1::cmpfunc_uint8_t (const void *a, const void *b)
{
   return ( *(uint8_t*)a - *(uint8_t*)b );
}

uint8_t CPowerManagementMk1::get_battery_percentage_from_voltage(float battery_voltage)
{
    /* Full  = 12.50v
       Empty = 10.50v (shutdown at this - still need to add h/w low voltage shutoff just below this)
     
       Limitations: When on charge, this will be >= 13v when almost full, but there's no definite way to 
                    tell when a charger is plugged in (i.e. < 13v doesn't mean the charger *isn't* plugged in)
     */
    float pc = ((battery_voltage - 10.5)/2) * 100;
    // printf("voltage = %f\n", battery_voltage);
    
    if (pc < 0)
        pc = 0;
    if (pc > 100)
        pc = 100;

    return (uint8_t)pc;
}

//////////// IPowerManagement ////////////

void CPowerManagementMk1::loop()
{

}

IPowerManagement::power_status_t CPowerManagementMk1::power_status()
{
    // MK1 can't tell for sure if it's plugged in or not
    return IPowerManagement::power_status_t::NA;
}

IPowerManagement::charging_status_t CPowerManagementMk1::charging_status()
{
    // MK1 can't tell for sure if it's charging or not
    return IPowerManagement::charging_status_t::NA;
}

bool CPowerManagementMk1::get_stat(int16_t* stat, power_stat_t type)
{
    switch (type)
    {
        case power_stat_t::BatVoltage:
            *stat = (_battery_voltage * 1000); // return millivolts
            return true;

        default:
            return false;
    }
}

uint8_t CPowerManagementMk1::get_battery_percentage()
{
    uint8_t readings_count = BAT_AVG_COUNT;

    if (_inital_startup)
    {
        if (_batt_reading_idx == 0)
        {
            printf("CPowerManagementMk1::get_battery_percentage(): called without any readings\n");
            return 0;
        }

        readings_count = _batt_reading_idx;
    }

    // copy readings to batt_percentage_sorted, then sort (don't want to interfere with _batt_percentage)
    uint8_t batt_percentage_sorted[BAT_AVG_COUNT] = {0};
    for(int x=0; x < BAT_AVG_COUNT; x++)
        batt_percentage_sorted[x] = _batt_percentage[x];
    qsort(batt_percentage_sorted, readings_count, sizeof(uint8_t), CPowerManagementMk1::cmpfunc_uint8_t);

    uint32_t total=0;
    uint8_t avg_count=0;
    // ignore lowest 10% of values. get the average of the rest
    for (uint8_t reading=readings_count/10; reading < readings_count; reading++)
    {
        avg_count++;
        total += batt_percentage_sorted[reading];
    }
    
    uint32_t avg = total/avg_count;
    return avg;
}

