#include "CUsbPower.h"
#include "pico/stdlib.h"
#include "../globals.h"

CUsbPower::CUsbPower(hw_variant_t variant)
{
    if (variant == hw_variant_t::V2_2)
    {
        _active = true;
        _charge_ctl.set_register_reset(true); // reset to defaults
        _charge_ctl.set_watchdog_reset(BQ25601::watchdog_reset_enum::RESET);
        _charge_ctl.read_all_registers();

        // If the BQ25601's watchdog resets it (which would happen some time after the zc95 is 
        // switched off, if not disabled), the charge current gets reset to the default of 2040 mA, 
        // but the input current limit does not always get reset.
        // So if the charge current was set quite low and the box was turned off, the charge current 
        // could jump above what it was set to. Disabling the watchdog should ensure that doesn't happen.
        // If the BQ25601 looses power entirely (e.g. low voltage shut-off), the current limit
        // will be reset to the default of 500 mA, so the max charge current is effectively limited 
        // to that, which should be fine.

        _charge_ctl.set_watchdog_time(BQ25601::watchdog_enum::WATCHDOG_DISABLED);
        set_charge_current();
    }
    else
    {
        _active = false;
    }
}

void CUsbPower::loop()
{
    if (!_active)
        return;

    update_input_current_limit(false);
    set_charge_current();

    _charge_ctl.read_register(BQ25601_REG00);
    uint16_t charge_controller_set_input_limit = _charge_ctl.get_input_current_limit_mA();
    if (_reported_current_limit_ma != charge_controller_set_input_limit)
    {
        printf("USB input current limit change: %d mA => %d mA\n", _reported_current_limit_ma, charge_controller_set_input_limit);  
        _reported_current_limit_ma = charge_controller_set_input_limit;
    }
}

bool CUsbPower::ext_power_good()
{
    if (!_active)
        return false;

    _charge_ctl.read_register(BQ25601_REG0A);
    return _charge_ctl.vbus_power_good();
}

BQ25601::charge_status_enum CUsbPower::charge_status()
{
    if (!_active)
        return BQ25601::charge_status_enum::NOT_CHARGING;

    _charge_ctl.read_register(BQ25601_REG08);
    return _charge_ctl.charge_status();
}

void CUsbPower::set_cc1_voltage_mV(int16_t mv)
{
    _cc1_voltage = mv;
}

void CUsbPower::set_cc2_voltage_mV(int16_t mv)
{
    _cc2_voltage = mv;
}

void CUsbPower::set_charge_current()
{
    _charge_ctl.read_register(BQ25601_REG02);
    uint16_t charge_current = _charge_ctl.get_fast_charge_current_mA();

    uint16_t configured_charge_current = g_SavedSettings->get_batt_charge_current_mA();

    if (charge_current != configured_charge_current)
    {
        printf("Changing battery charge current: %d mA => %d mA\n", charge_current, configured_charge_current);
        _charge_ctl.set_fast_charge_current_mA(configured_charge_current);
    }
}

void CUsbPower::update_usb_power_status()
{
    usb_power_t usb_before = _usb_power;
    _usb_power = get_usb_power();

    if (usb_before != _usb_power)
    {
        printf("Detected USB charger changed: %s => %s\n",  usb_power_status_string(usb_before).c_str(), usb_power_status_string(_usb_power).c_str());
        _time_usb_power_changed = time_us_64();
    }
}

void CUsbPower::update_input_current_limit(bool force_update)
{
    update_usb_power_status();

    if (force_update || (_time_usb_power_changed && (time_us_64() - _time_usb_power_changed) > SecondsInUs(2)))
    {
        uint16_t limit = get_current_limit_ma(_usb_power);
        _charge_ctl.set_input_current_limit_mA(limit);
        printf("Setting input current limit to: %d mA\n", limit);
        _time_usb_power_changed = 0;
    }
}

// Use the voltage on the USB CC pins to figure out what type of charger is attached.
// Can be fooled by dubious cables.
CUsbPower::usb_power_t CUsbPower::get_usb_power()
{
    int16_t cc = _cc1_voltage;
    if (_cc2_voltage > cc)
        cc = _cc2_voltage;

    if (cc > 1310) // 1.31v
        return usb_power_t::CURRENT_3A;
    else if (cc > 700)  // 0.7v
        return usb_power_t::CURRENT_1_5A;
    else if (cc > 250) // 0.25v Some USB-A -> USB-C cables seem to do this
        return usb_power_t::CURRENT_1A;
    else
        return usb_power_t::UNKNOWN;
}

uint16_t CUsbPower::get_current_limit_ma(usb_power_t usb)
{
    switch (usb)
    {
        case usb_power_t::CURRENT_3A:   return 3000;
        case usb_power_t::CURRENT_1_5A: return 1500;
        case usb_power_t::CURRENT_1A:   return 1000;
        default:                        return  500;
    }
}

std::string CUsbPower::usb_power_status_string(usb_power_t usb_power_status)
{
    switch (usb_power_status)
    {
        case usb_power_t::CURRENT_1A:       return "1A";
        case usb_power_t::CURRENT_1_5A:     return "1.5A";
        case usb_power_t::CURRENT_3A:       return "3A";
        default:                            return "Unknown";
    }
}
