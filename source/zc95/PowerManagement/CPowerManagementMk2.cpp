#include "CPowerManagementMk2.h"
#include <string.h>

CPowerManagementMk2::CPowerManagementMk2(CMainBoardPortExp* mainboard_port_exp)
{
    _mainboard_port_exp = mainboard_port_exp;
    BQ27441_ctx_t _BQ27441 = {
            .BQ27441_i2c_address = BQ72441_I2C_ADDRESS,
            .write_reg = s_BQ27441_i2cWriteBytes,       // i2c write callback 
            .read_reg = s_BQ27441_i2cReadBytes          // i2c read callback 
    };

    _BQ27441_init_success = BQ27441_init(&_BQ27441);
    if (_BQ27441_init_success)
    {
        printf("CPowerManagementMk2(): BQ27441 init OK\n");
        if (BQ27441_itporFlag())
        {
            // The BQ27441 remains powered with the main switch off, and even if the zc95's low voltage protection has switched off power.
            // So if it's lost power, it probably means that either the battery has been removed, or the battery's in built protection
            // has been triggered somehow. Need to reconfigure the BQ27441 so it knows what size battery to expect, as this is lost
            // without power. Despite this, the battery gauge won't be great until a full discharge / charge cycle has been completed.
            printf("CPowerManagementMk2(): BQ27441 ITPOR flag is set, reloading inital config (device has lost power)\n");

            BQ27441_enterConfig(true);
            const uint16_t battery_capacity_mah = 5300;
            const uint16_t tp4056_charge_current_ma = 780;

            BQ27441_setCapacity(battery_capacity_mah);
            BQ27441_setDesignEnergy((float)battery_capacity_mah * 3.7);
            BQ27441_setTerminateVoltageMin(2900); // From U14 / HY2111-GB

            // Mostly from "Quickstart Guide for bq27441-G1" (SLUUAP7)
            // Also TP4056's terminate charge when current drops below 10% of the programmed charge current
            uint16_t taper_rate = (float)battery_capacity_mah / (0.1f * (((float)tp4056_charge_current_ma/10.0f) * 1.15f));
            BQ27441_setTaperRateTime(taper_rate);

            BQ27441_exitConfig(true);
        }

        loop();
    }
    else
        printf("CPowerManagementMk2(): BQ27441 init FAILURE\n");
}

void CPowerManagementMk2::print_status()
{
    printf("***********************\n");
    printf("* BAT: Voltage   : %d mV\n" , BQ27441_voltage());
    printf("* BAT: Current   : %d mA\n" , BQ27441_current(current_measure::AVG));
    printf("* BAT: SoC       : %d%%\n"  , BQ27441_soc(soc_measure::FILTERED));
    printf("* BAT: cap remain: %d mAh\n", BQ27441_capacity(capacity_measure::REMAIN));

    printf("* CHG: %s\n", _mainboard_port_exp->get_tp4056_charge_status()  ? "YES" : "NO");
    printf("* STB: %s\n", _mainboard_port_exp->get_tp4056_standby_status() ? "YES" : "NO");
}

int16_t CPowerManagementMk2::s_BQ27441_i2cWriteBytes(uint8_t DevAddress, uint8_t subAddress, uint8_t* src, uint8_t count)
{
    uint8_t buffer[10] = {0};
    buffer[0] = subAddress; // register
    if (count > sizeof(buffer)-1)
        return false;

    memcpy(buffer+1, src, count);

    return (i2c_write(__func__, DevAddress, buffer, count+1, false) > 0);
}

int16_t CPowerManagementMk2::s_BQ27441_i2cReadBytes(uint8_t DevAddress, uint8_t subAddress, uint8_t* dest, uint8_t count)
{
    uint8_t data[1];
    data[0] = subAddress;
    int bytes_written = i2c_write(__func__, DevAddress, data, sizeof(data), false);
    if (bytes_written != sizeof(data))
    {
        printf("s_BQ27441_i2cReadBytes::read failed! i2c bytes_written = %d\n", bytes_written);
        return false;
    }

    return (i2c_read(__func__, DevAddress, dest, count, false) == count);
}

void CPowerManagementMk2::add_raw_adc_readings(const uint8_t *raw_adc_readings_buffer, uint8_t buffer_array_len)
{
    uint32_t readings[10];
    uint8_t readings_len = sizeof(readings) / sizeof(readings[0]);

    // On initial startup all the readings in _batt_percentage will be 0, so try and set them to something better
    // as quick as possible.
    if (_inital_startup || (time_us_64() - _last_vbus_update > 1000000)) // 1sec
    {
        if (buffer_array_len < readings_len)
        {
            printf("CPowerManagementMk2::add_raw_adc_readings: need at least %d readings\n", readings_len);
            return;
        }
        _inital_startup = false;
        
        // qsort sorts an array, but we don't want to alter the raw_adc_readings_buffer input array, so copy.
        for (uint8_t x=0; x < readings_len; x++)
            readings[x] = raw_adc_readings_buffer[x];

        // ignore 2 highest and 2 lowest values. get the average of the rest
        qsort(readings, readings_len, sizeof(uint32_t), CPowerManagementMk2::s_cmpfunc);
        uint32_t total=0;
        for (uint8_t reading=2; reading < readings_len-2; reading++)
            total += readings[reading];

        float avg = (float)total/(float)(readings_len-4);

        // Convert average ADC reading to voltage seen at ADC input
        const float conversion_factor = 3.3f / (1 << 8);
        float adc_voltage = avg * conversion_factor;
        
        // Convert to millivolts. Using a 100k-100k voltage divider, so double the voltage, then x1000 for V -> mV
        _vbus_voltage = (adc_voltage * 2) * 1000;

        _last_vbus_update = time_us_64();
    }
}

int CPowerManagementMk2::s_cmpfunc (const void *a, const void *b)
{
   return ( *(uint32_t*)a - *(uint32_t*)b );
}

int CPowerManagementMk2::s_cmpfunc_uint8_t (const void *a, const void *b)
{
   return ( *(uint8_t*)a - *(uint8_t*)b );
}


//////////// IPowerManagement ////////////

void CPowerManagementMk2::loop()
{
    // update stats at most every 1 second
    if (time_us_64() - _last_batt_param_refresh > (1000 * 1000) || _last_batt_param_refresh == 0)
    {

        _battery_percentage = BQ27441_soc(soc_measure::FILTERED);
        _battery_voltage = BQ27441_voltage();
        _current_mA = BQ27441_current(current_measure::AVG); // -ve is power drawn from battery, +ve is change current into battery
        _remaining_capacity_mah = BQ27441_capacity(capacity_measure::REMAIN);
        _full_capacity_mah = BQ27441_capacity(capacity_measure::FULL_F);

        if (_current_mA > 0)
        {
            _charging_status = charging_status_t::Charging;
            _power_status = power_status_t::OnExternalPower;
        }
        else if (_current_mA == 0)
        {
            // If nothing's coming out of the battery, must be on external power
            _power_status = power_status_t::OnExternalPower;

            if (_battery_percentage == 100)
            {
                _charging_status = charging_status_t::Charged; 
            }
            else
            {
                if (_mainboard_port_exp->get_tp4056_charge_status())
                {
                    _charging_status = charging_status_t::Charging; 
                }
                else
                {
                    // On external power, battery isn't charged, but also isn't charging
                    _charging_status = charging_status_t::Unknown; 
                }
            }
        }
        else // current < 0 i.e. battery is discharging
        {
            _charging_status = charging_status_t::Unknown;
            _power_status = power_status_t::OnBattery;
        }

        _last_batt_param_refresh = time_us_64();

        // print_status();
    }
}

IPowerManagement::power_status_t CPowerManagementMk2::power_status()
{
    if (!_BQ27441_init_success)
    {
        return IPowerManagement::power_status_t::Unknown;
    }
    
    return _power_status;
}

IPowerManagement::charging_status_t CPowerManagementMk2::charging_status()
{
    if (!_BQ27441_init_success)
    {
        return IPowerManagement::charging_status_t::Unknown;
    }

    return _charging_status;
}

bool CPowerManagementMk2::get_stat(int16_t* stat, power_stat_t type)
{
    if (!_BQ27441_init_success)
    {
        return false;
    }

    switch (type)
    {
        case power_stat_t::BatCurrent:
            *stat = _current_mA;
            return true;

        case power_stat_t::BatVoltage:
            *stat = _battery_voltage;
            return true;

        case power_stat_t::RemainingCapacity:
            *stat = _remaining_capacity_mah;
            return true;

        case power_stat_t::FullCapacity:
            *stat = _full_capacity_mah;
            return true;
            
        case power_stat_t::VbusVoltage:
            *stat = _vbus_voltage;
            return true;

        default:
            return false;
    }
}

uint8_t CPowerManagementMk2::get_battery_percentage()
{
    if (!_BQ27441_init_success)
    {
        return 0xFF;
    }

    return _battery_percentage;
}

