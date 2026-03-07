/**
 *     Driver for: BQ25601D I2C Controlled 3-A Single-Cell Battery Charger 
 *                 With USB Charger Detection for High Input Voltage and 
 *                 Narrow Voltage
 *
 * Copyright (c) 2025 CrashOverride85, https://github.com/CrashOverride85
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met: 
 *
 *  * Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer. 
 *  * Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY 
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
 * DAMAGE. 
 */

 #include <stddef.h>
 #include <stdio.h>
 #include "BQ25601.h"
 #include "../CUtil.h"
 #include "../../config.h"
 
BQ25601::BQ25601()
{

}

BQ25601::~BQ25601()
{

}

void BQ25601::read_register(uint8_t reg)
{
    uint8_t data[1];
    data[0] = reg;

    if (i2c_write(__func__, BQ25601_CHARGE_CONTROLLER, data, sizeof(data), false) == 1)
    {
        int ret = i2c_read(__func__, BQ25601_CHARGE_CONTROLLER, data, 1, false);

        if (ret == 1)
            _register[reg] = data[0];
    }
}

void BQ25601::read_all_registers()
{
    for(uint8_t reg=0; reg < BQ25601_REG_MAX; reg++)
        read_register(reg);
}


//////////////////////////////// REG00 ////////////////////////////////

bool BQ25601::get_hiz()
{
    return (_register[BQ25601_REG00] & 0x80) != 0;
}

void BQ25601::set_hiz(bool value)
{
    read_register(BQ25601_REG00);

    uint8_t new_reg00 = _register[BQ25601_REG00] & ~(0x80);
    if (value)
    {
        new_reg00 |= 0x80;
    }
    set_register(BQ25601_REG00, new_reg00);
    read_register(BQ25601_REG00);
}

bool BQ25601::get_stat_pin()
{
    // EN_ICHG_MON is two bits, but only 00 (enabled) and 11 (disabled) are used

    uint8_t en_ichg_mon = (_register[BQ25601_REG00] >> 5) & 0x03;
    return (en_ichg_mon == 0);
}

void BQ25601::set_stat_pin(bool value)
{
    // EN_ICHG_MON is two bits, but only 00 (enabled) and 11 (disabled) are used
    read_register(BQ25601_REG00);

    uint8_t new_reg00 = _register[BQ25601_REG00] & 0x9F;
    if (!value)
    {
        new_reg00 |= 0x60;
    }
    
    set_register(BQ25601_REG00, new_reg00);
    read_register(BQ25601_REG00);
}

uint16_t BQ25601::get_input_current_limit_mA()
{
    const uint16_t offset = 100;
    uint16_t iindpm = (_register[BQ25601_REG00] & 0x1F);
    return (iindpm * 100) + offset;
}

uint16_t BQ25601::set_input_current_limit_mA(uint16_t lim_mA)
{
    const uint16_t offset = 100;

    if ((lim_mA < offset) || (lim_mA > 3200))
        return 0;

    lim_mA -= offset;
    uint8_t new_iindpm = lim_mA / 100;

    read_register(BQ25601_REG00);
    uint8_t new_reg00 = _register[BQ25601_REG00] & 0xE0;
    new_reg00 |= new_iindpm;
    set_register(BQ25601_REG00, new_reg00);

    read_register(BQ25601_REG00);
    return get_input_current_limit_mA();
}


//////////////////////////////// REG01 ////////////////////////////////
 

bool BQ25601::get_pfm_enabled()
{
    return (_register[BQ25601_REG01] & 0x80) == 0;
}

void BQ25601::set_pfm_enabled(bool value)
{
    read_register(BQ25601_REG01);

    uint8_t new_reg01 = _register[BQ25601_REG01] & ~(0x80);
    /* From datasheet:
    *   0 – Enable PFM
    *   1 – Disable PFM
    */
    if (!value) 
    {
        new_reg01 |= 0x80;
    }
    set_register(BQ25601_REG01, new_reg01);
    read_register(BQ25601_REG01);
}

BQ25601::watchdog_reset_enum BQ25601::get_watchdog_reset()
{
    if ((_register[BQ25601_REG01] & 0x40) == 0)
        return watchdog_reset_enum::NORMAL;
    else
        return watchdog_reset_enum::RESET;
}

void BQ25601::set_watchdog_reset(watchdog_reset_enum value)
{
    read_register(BQ25601_REG01);

    uint8_t new_reg01 = _register[BQ25601_REG01] & ~(0x40);
    if (value == watchdog_reset_enum::RESET) 
    {
        new_reg01 |= 0x40;
    }
    set_register(BQ25601_REG01, new_reg01);
    read_register(BQ25601_REG01);
}

bool BQ25601::get_otg_enabled()
{
    return (_register[BQ25601_REG01] & 0x20) != 0;
}

void BQ25601::set_otg_enabled(bool value)
{
    read_register(BQ25601_REG01);

    uint8_t new_reg01 = _register[BQ25601_REG01] & ~(0x20);
    if (value) 
    {
        new_reg01 |= 0x20;
    }
    set_register(BQ25601_REG01, new_reg01);
    read_register(BQ25601_REG01);
}

bool BQ25601::get_charge_enabled()
{
    return (_register[BQ25601_REG01] & 0x10) != 0;
}

void BQ25601::set_charge_enabled(bool value)
{
    read_register(BQ25601_REG01);

    uint8_t new_reg01 = _register[BQ25601_REG01] & ~(0x10);
    if (value) 
    {
        new_reg01 |= 0x10;
    }
    set_register(BQ25601_REG01, new_reg01);
    read_register(BQ25601_REG01);
}

BQ25601::min_sys_volt_enum BQ25601::get_min_sys_voltage()
{
    return (BQ25601::min_sys_volt_enum)((_register[BQ25601_REG01] & 0x0E) >> 1);
}

void BQ25601::set_min_sys_voltage(min_sys_volt_enum value)
{
    read_register(BQ25601_REG01);

    uint8_t new_reg01 = _register[BQ25601_REG01] & ~(0x0E);

    new_reg01 |= ((uint8_t)value << 1);
    
    set_register(BQ25601_REG01, new_reg01);
    read_register(BQ25601_REG01);
}


 //////////////////////////////// REG02 ////////////////////////////////

bool BQ25601::get_boost_lim()
{
    return (_register[BQ25601_REG02] & 0x80) != 0;
}

void BQ25601::set_boost_lim(bool value)
{
    read_register(BQ25601_REG02);

    uint8_t new_reg02 = _register[BQ25601_REG02] & ~(0x80);
    if (value) 
    {
        new_reg02 |= 0x80;
    }
    set_register(BQ25601_REG02, new_reg02);
    read_register(BQ25601_REG02);
}

bool BQ25601::get_q1_full_on()
{
    return (_register[BQ25601_REG02] & 0x40) != 0;
}

void BQ25601::set_q1_full_on(bool value)
{
    read_register(BQ25601_REG02);

    uint8_t new_reg02 = _register[BQ25601_REG02] & ~(0x40);
    if (value) 
    {
        new_reg02 |= 0x40;
    }
    set_register(BQ25601_REG02, new_reg02);
    read_register(BQ25601_REG02);
}

uint16_t BQ25601::set_fast_charge_current_mA(uint16_t current_mA)
{
    if (current_mA > 3000)
        return 0;

    uint16_t ichg = current_mA / 60;

    read_register(BQ25601_REG02);
    uint8_t new_reg02 = _register[BQ25601_REG02] & ~(0x3F);
    new_reg02 |= ichg;
    set_register(BQ25601_REG02, new_reg02);

    read_register(BQ25601_REG02);
    return get_fast_charge_current_mA();
}

uint16_t BQ25601::get_fast_charge_current_mA()
{
    return (_register[BQ25601_REG02] & 0x3F) * 60;
}

 
//////////////////////////////// REG03 ////////////////////////////////
 

uint16_t BQ25601::get_pre_charge_current_mA()
{
    const uint8_t offset = 60;
    return (((_register[BQ25601_REG03] & 0xF0) >> 4) * 60) + offset;
}

uint16_t BQ25601::set_pre_charge_current_mA(uint16_t current_mA)
{
    const uint8_t offset = 60;

    if ((current_mA < 60) || (current_mA > 780))
        return 0;

    uint16_t iprechg = (current_mA - offset) / 60;

    read_register(BQ25601_REG03);
    uint8_t new_reg03 = _register[BQ25601_REG03] & ~(0xF0);
    new_reg03 |= (iprechg << 4);
    set_register(BQ25601_REG03, new_reg03);

    read_register(BQ25601_REG03);
    return get_pre_charge_current_mA();
}

uint16_t BQ25601::get_charge_termination_current_mA()
{
    const uint8_t offset = 60;
    return ((_register[BQ25601_REG03] & 0x0F) * 60) + offset;
}

uint16_t BQ25601::set_charge_termination_current_mA(uint16_t current_mA)
{
    const uint8_t offset = 60;

    if ((current_mA < 60) || (current_mA > 780))
        return 0;

    uint16_t iterm = (current_mA - offset) / 60;

    read_register(BQ25601_REG03);
    uint8_t new_reg03 = _register[BQ25601_REG03] & ~(0x0F);
    new_reg03 |= iterm;
    set_register(BQ25601_REG03, new_reg03);

    read_register(BQ25601_REG03);
    return get_charge_termination_current_mA();
}
 
//////////////////////////////// REG04 ////////////////////////////////

uint16_t BQ25601::get_vreg_mV()
{
    uint8_t reg_value = _register[BQ25601_REG04];
    return get_vreg_mV_from_reg(reg_value);
}

uint16_t BQ25601::set_vreg_mV(uint16_t vreg_mV)
{
    if (vreg_mV < 3847 || vreg_mV > 4615)
        return 0;

    uint16_t offset = vreg_mV - 3847;
    uint16_t vreg = offset >> 5;
    uint8_t reg_value = vreg << 3;
    
    uint16_t set_to = get_vreg_mV_from_reg(reg_value);;

    read_register(BQ25601_REG04);
    uint8_t new_reg04 = _register[BQ25601_REG04] & 0x07;
    new_reg04 |= reg_value;

    set_register(BQ25601_REG04, new_reg04);
    read_register(BQ25601_REG04);
    return set_to;
}

BQ25601::topoff_timer_enum BQ25601::get_topoff_timer()
{
    return (topoff_timer_enum)((_register[BQ25601_REG04] >> 1) & 0x03);
}

void BQ25601::set_topoff_timer(BQ25601::topoff_timer_enum value)
{
    read_register(BQ25601_REG04);
    uint8_t topoff_timer = (uint8_t)value << 1;
    
    uint8_t new_reg04 = _register[BQ25601_REG04] & 0xF9;
    new_reg04 |= topoff_timer;
    set_register(BQ25601_REG04, new_reg04);
    read_register(BQ25601_REG04);
}

BQ25601::rechg_thresh_enum BQ25601::get_recharge_thresh()
{
    return (rechg_thresh_enum)(_register[BQ25601_REG04] & 0x01);
}

void BQ25601::set_recharge_thresh(rechg_thresh_enum value)
{
    read_register(BQ25601_REG04);

    uint8_t new_reg04 = _register[BQ25601_REG04] & ~(0x01);
    if (value == rechg_thresh_enum::VRECHG_200mV)
    {
        new_reg04 |= 0x01;
    }
    set_register(BQ25601_REG04, new_reg04);
    read_register(BQ25601_REG04);
}

 
//////////////////////////////// REG05 ////////////////////////////////
 

bool BQ25601::get_enable_termination()
{
    return (_register[BQ25601_REG05] & 0x80) != 0;
}

void BQ25601::set_enable_termination(bool value)
{
    read_register(BQ25601_REG05);

    uint8_t new_reg05 = _register[BQ25601_REG05] & ~(0x80);
    if (value) 
    {
        new_reg05 |= 0x80;
    }
    set_register(BQ25601_REG05, new_reg05);
    read_register(BQ25601_REG05);
}

BQ25601::watchdog_enum BQ25601::get_watchdog_time()
{
    return (watchdog_enum)((_register[BQ25601_REG05] & 0x30) >> 4);
}

void BQ25601::set_watchdog_time(watchdog_enum value)
{
    read_register(BQ25601_REG05);

    uint8_t new_reg05 = _register[BQ25601_REG05] & ~(0x30);
    new_reg05 |= ((uint8_t)value << 4);
    set_register(BQ25601_REG05, new_reg05);
    read_register(BQ25601_REG05);
}

bool BQ25601::get_enable_timer()
{
    return (_register[BQ25601_REG05] & 0x08) != 0;
}

void BQ25601::set_enable_timer(bool value)
{
    read_register(BQ25601_REG05);

    uint8_t new_reg05 = _register[BQ25601_REG05] & ~(0x08);
    if (value) 
    {
        new_reg05 |= 0x08;
    }
    set_register(BQ25601_REG05, new_reg05);
    read_register(BQ25601_REG05);
}

BQ25601::charge_time_enum BQ25601::get_charge_timer()
{
    return (charge_time_enum)((_register[BQ25601_REG05] & 0x04) >> 2);
}

void BQ25601::set_charge_timer(charge_time_enum value)
{
    read_register(BQ25601_REG05);

    uint8_t new_reg05 = _register[BQ25601_REG05] & ~(0x04);
    if (value == charge_time_enum::TIMER_10_HRS) 
    {
        new_reg05 |= 0x04;
    }
    set_register(BQ25601_REG05, new_reg05);
    read_register(BQ25601_REG05);
}

BQ25601::thermal_reg_enum BQ25601::get_thermal_reg_temp()
{
    return (thermal_reg_enum)((_register[BQ25601_REG05] & 0x02) >> 1);
}

void BQ25601::set_thermal_reg_temp(thermal_reg_enum value)
{
    read_register(BQ25601_REG05);

    uint8_t new_reg05 = _register[BQ25601_REG05] & ~(0x02);
    if (value == thermal_reg_enum::TREG_110C) 
    {
        new_reg05 |= 0x02;
    }
    set_register(BQ25601_REG05, new_reg05);
    read_register(BQ25601_REG05);
}

//////////////////////////////// REG06 ////////////////////////////////

 BQ25601::ovp_enum BQ25601::get_ovp()
{
    return (ovp_enum)((_register[BQ25601_REG06] & 0xC0) >> 6);
}

void BQ25601::set_ovp(ovp_enum value)
{
    read_register(BQ25601_REG06);

    uint8_t new_reg06 = _register[BQ25601_REG06] & ~(0xC0);
    new_reg06 |= ((uint8_t)value << 6);
    set_register(BQ25601_REG06, new_reg06);
    read_register(BQ25601_REG06);
}

BQ25601::boost_voltage_enum BQ25601::get_boost_voltage()
{
    return (boost_voltage_enum)((_register[BQ25601_REG06] & 0x30) >> 4);
}

void BQ25601::set_boost_voltage(boost_voltage_enum value)
{
    read_register(BQ25601_REG06);

    uint8_t new_reg06 = _register[BQ25601_REG06] & ~(0x30);
    new_reg06 |= ((uint8_t)value << 4);
    set_register(BQ25601_REG06, new_reg06);
    read_register(BQ25601_REG06);
}

uint16_t BQ25601::get_absolute_vindpm_threshold_mV()
{
    const uint16_t offset_mV = 3900;
    uint8_t vindpm = _register[BQ25601_REG06] & 0x0F;
    uint16_t mV = (vindpm * 100) + offset_mV;

    return mV;
}

uint16_t BQ25601::set_absolute_vindpm_threshold_mV(uint16_t mV)
{
    const uint16_t offset_mV = 3900;

    if (mV < offset_mV || mV > 5400)
        return 0;

    uint16_t vindpm = (mV - offset_mV) / 100;

    read_register(BQ25601_REG06);
    uint8_t new_reg06 = _register[BQ25601_REG06] & ~(0x0F);
    new_reg06 |= vindpm;
    set_register(BQ25601_REG06, new_reg06);
    read_register(BQ25601_REG06);

    return get_absolute_vindpm_threshold_mV();
}


//////////////////////////////// REG07 ////////////////////////////////

bool BQ25601::get_in_current_lim_detection()
{
    return ((_register[BQ25601_REG07] & 0x80) != 0);
}

void BQ25601::set_in_current_lim_detection(bool value)
{
    read_register(BQ25601_REG07);

    uint8_t new_reg07 = _register[BQ25601_REG07] & ~(0x80);
    if (value) 
    {
        new_reg07 |= 0x80;
    }
    set_register(BQ25601_REG07, new_reg07);
    read_register(BQ25601_REG07);
}

bool BQ25601::get_x2_slow_safety_timer()
{
    return ((_register[BQ25601_REG07] & 0x40) != 0);
}

void BQ25601::set_x2_slow_safety_timer(bool value)
{
    read_register(BQ25601_REG07);

    uint8_t new_reg07 = _register[BQ25601_REG07] & ~(0x40);
    if (value) 
    {
        new_reg07 |= 0x40;
    }
    set_register(BQ25601_REG07, new_reg07);
    read_register(BQ25601_REG07);
}

bool BQ25601::get_batfet_disable()
{
    return ((_register[BQ25601_REG07] & 0x20) != 0);
}

void BQ25601::set_batfet_disable(bool value)
{
    read_register(BQ25601_REG07);

    uint8_t new_reg07 = _register[BQ25601_REG07] & ~(0x20);
    if (value) 
    {
        new_reg07 |= 0x20;
    }
    set_register(BQ25601_REG07, new_reg07);
    read_register(BQ25601_REG07);
}

bool BQ25601::get_batfet_delay()
{
    return ((_register[BQ25601_REG07] & 0x08) != 0);
}

void BQ25601::set_batfet_delay(bool value)
{
    read_register(BQ25601_REG07);

    uint8_t new_reg07 = _register[BQ25601_REG07] & ~(0x08);
    if (value) 
    {
        new_reg07 |= 0x08;
    }
    set_register(BQ25601_REG07, new_reg07);
    read_register(BQ25601_REG07);
}

bool BQ25601::get_batfet_reset_enable()
{
    return ((_register[BQ25601_REG07] & 0x04) != 0);
}

void BQ25601::set_batfet_reset_enable(bool value)
{
    read_register(BQ25601_REG07);

    uint8_t new_reg07 = _register[BQ25601_REG07] & ~(0x04);
    if (value) 
    {
        new_reg07 |= 0x04;
    }
    set_register(BQ25601_REG07, new_reg07);
    read_register(BQ25601_REG07);
}

BQ25601::vdpm_bat_track_enum BQ25601::get_vdpm_bat_track()
{
    return (vdpm_bat_track_enum)((_register[BQ25601_REG07] & 0x03));
}

void BQ25601::set_vdpm_bat_track(vdpm_bat_track_enum value)
{
    read_register(BQ25601_REG07);

    uint8_t new_reg07 = _register[BQ25601_REG07] & ~(0x03);
    new_reg07 |= (uint8_t)value;
    set_register(BQ25601_REG07, new_reg07);
    read_register(BQ25601_REG07);
}


//////////////////////////////// REG08 ////////////////////////////////
 
BQ25601::vbus_status_enum BQ25601::vbus_source()
{
    return (vbus_status_enum)(_register[BQ25601_REG08] >> 5);
}

BQ25601::charge_status_enum BQ25601::charge_status()
{
    return (charge_status_enum)((_register[BQ25601_REG08] >> 3) & 0x03);
}

bool BQ25601::power_good()
{
    return (_register[BQ25601_REG08] & 0x04) != 0;
}

bool BQ25601::thermal_status()
{
    return (_register[BQ25601_REG08] & 0x02) != 0;
}

bool BQ25601::vsys_reg()
{
    return (_register[BQ25601_REG08] & 0x01) != 0;
}


//////////////////////////////// REG09 ////////////////////////////////

bool BQ25601::watchdog_fault()
{
    return (_register[BQ25601_REG09] & 0x80) != 0;
}

bool BQ25601::boost_fault()
{
    return (_register[BQ25601_REG09] & 0x40) != 0;
}

BQ25601::charge_fault_enum BQ25601::charge_fault()
{
    return (charge_fault_enum)((_register[BQ25601_REG09] >> 4) & 0x03);
}

bool BQ25601::bat_fault()
{
    return (_register[BQ25601_REG09] & 0x08) != 0;
}

BQ25601::ntc_fault_enum BQ25601::ntc_fault()
{
    return (ntc_fault_enum)(_register[BQ25601_REG09] & 0x07);
}

//////////////////////////////// REG0A ////////////////////////////////


bool BQ25601::vbus_power_good()
{
    return (_register[BQ25601_REG0A] & 0x80) != 0;
}

bool BQ25601::in_vindpm()
{
    return (_register[BQ25601_REG0A] & 0x40) != 0;
}

bool BQ25601::in_iindpm()
{
    return (_register[BQ25601_REG0A] & 0x20) != 0;
}

bool BQ25601::top_off_active()
{
    return (_register[BQ25601_REG0A] & 0x08) != 0;
}

bool BQ25601::input_over_voltage()
{
    return (_register[BQ25601_REG0A] & 0x04) != 0;
}

bool BQ25601::get_mask_vindpm_int()
{
    return (_register[BQ25601_REG0A] & 0x02) != 0;
}

void BQ25601::set_mask_vindpm_int(bool value)
{
    read_register(BQ25601_REG0A);

    uint8_t new_reg0A = _register[BQ25601_REG0A] & ~(0x02);
    if (value) 
    {
        new_reg0A |= 0x02;
    }
    set_register(BQ25601_REG0A, new_reg0A);
    read_register(BQ25601_REG0A);
}

bool BQ25601::get_mask_iindpm_int()
{
    return (_register[BQ25601_REG0A] & 0x01) != 0;
}

void BQ25601::set_mask_iindpm_int(bool value)
{
    read_register(BQ25601_REG0A);

    uint8_t new_reg0A = _register[BQ25601_REG0A] & ~(0x01);
    if (value) 
    {
        new_reg0A |= 0x01;
    }
    set_register(BQ25601_REG0A, new_reg0A);
    read_register(BQ25601_REG0A);
}
 
//////////////////////////////// REG0B ////////////////////////////////


bool BQ25601::get_register_reset()
{
    return (_register[BQ25601_REG0B] & 0x80) != 0;
}

void BQ25601::set_register_reset(bool value)
{
    read_register(BQ25601_REG0B);

    uint8_t new_reg0A = _register[BQ25601_REG0B] & ~(0x80);
    if (value) 
    {
        new_reg0A |= 0x80;
    }
    set_register(BQ25601_REG0B, new_reg0A);
    read_register(BQ25601_REG0B);
}

BQ25601::pn_enum BQ25601::pn()
{
    return (pn_enum)((_register[BQ25601_REG0B] & 0x78) >> 3);
}


/* Private */

uint16_t BQ25601::get_vreg_mV_from_reg(uint8_t reg_value)
{
    uint16_t mV = 3847;

    if (reg_value & 0x80) mV += 512;
    if (reg_value & 0x40) mV += 256;
    if (reg_value & 0x20) mV += 128;
    if (reg_value & 0x10) mV += 64;
    if (reg_value & 0x08) mV += 32;

    // From datasheet: "Special Value: (01111): 4.343 V"
    if ((reg_value >> 3) == 0x0F)
        mV = 4343;

    return mV;
}


void BQ25601::set_register(uint8_t reg, uint8_t value)
{
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    
    i2c_write(__func__, BQ25601_CHARGE_CONTROLLER, data, sizeof(data), false);
}
