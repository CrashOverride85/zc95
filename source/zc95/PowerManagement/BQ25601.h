/**
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


 #ifndef _BQ25601_H
 #define _BQ25601_H
 

 #define BQ25601_REG00 0x00
 #define BQ25601_REG01 0x01
 #define BQ25601_REG02 0x02
 #define BQ25601_REG03 0x03
 #define BQ25601_REG04 0x04
 #define BQ25601_REG05 0x05
 #define BQ25601_REG06 0x06
 #define BQ25601_REG07 0x07
 #define BQ25601_REG08 0x08
 #define BQ25601_REG09 0x09
 #define BQ25601_REG0A 0x0A
 #define BQ25601_REG0B 0x0b
 #define BQ25601_REG_MAX BQ25601_REG0B

 class BQ25601 
 {
     public:
        BQ25601();
        ~BQ25601();

        void read_register(uint8_t reg);
        void read_all_registers();
        
        // Reg 00
        bool get_hiz();
        void set_hiz(bool value);
        bool get_stat_pin();
        void set_stat_pin(bool value);
        uint16_t get_input_current_limit_mA();
        uint16_t set_input_current_limit_mA(uint16_t lim_mA);       
        
        // Reg 01
        enum class watchdog_reset_enum  // for BQ25601_REG01, WD_RST
        {
            NORMAL   = 0x00,
            RESET    = 0x01
        };

        enum class min_sys_volt_enum  // for BQ25601_REG01, SYS_Min
        {
            MIN_2_6V   = 0x00,
            MIN_2_8V   = 0x01,
            MIN_3_0V   = 0x02,
            MIN_3_2V   = 0x03,
            MIN_3_4V   = 0x04,
            MIN_3_5V   = 0x05,
            MIN_3_6V   = 0x06,
            MIN_3_7V   = 0x07
        };

        bool get_pfm_enabled();
        void set_pfm_enabled(bool value);

        watchdog_reset_enum get_watchdog_reset();
        void set_watchdog_reset(watchdog_reset_enum value);
        bool get_otg_enabled();
        void set_otg_enabled(bool value);
        bool get_charge_enabled();
        void set_charge_enabled(bool value);
        min_sys_volt_enum get_min_sys_voltage();
        void set_min_sys_voltage(min_sys_volt_enum value);

        // Reg 02
        bool get_boost_lim();
        void set_boost_lim(bool value);
        bool get_q1_full_on();
        void set_q1_full_on(bool value);
        uint16_t set_fast_charge_current_mA(uint16_t current_mA);
        uint16_t get_fast_charge_current_mA();

        // Reg 03
        uint16_t get_pre_charge_current_mA();
        uint16_t set_pre_charge_current_mA(uint16_t current_mA);
        uint16_t get_charge_termination_current_mA();
        uint16_t set_charge_termination_current_mA(uint16_t current_mA);
        
        // Reg 04
        enum class topoff_timer_enum // for BQ25601_REG04, TOPOFF_TIMER
        {
            TIMER_DISABLED   = 0x00,
            TIMER_15_MINUTES = 0x01, 
            TIMER_30_MINUTES = 0x02,
            TIMER_45_MINUTES = 0x03
        };
        
        enum class rechg_thresh_enum // for BQ25601_REG04, VRECHG
        {
            VRECHG_100mV    = 0x00,
            VRECHG_200mV    = 0x01
        };

        uint16_t get_vreg_mV();
        uint16_t set_vreg_mV(uint16_t vreg_mV);
        topoff_timer_enum get_topoff_timer();
        void set_topoff_timer(topoff_timer_enum value);
        rechg_thresh_enum get_recharge_thresh();
        void set_recharge_thresh(rechg_thresh_enum value);

        // Reg 05
        enum class watchdog_enum  // for BQ25601_REG05, WATCHDOG
        {
            WATCHDOG_DISABLED   = 0x00,
            WATCHDOG_40S        = 0x01,
            WATCHDOG_80S        = 0x02,
            WATCHDOG_160S       = 0x03
        };
        
        enum class charge_time_enum  // for BQ25601_REG05, CHG_TIMER
        {
            TIMER_5_HRS   = 0x00,
            TIMER_10_HRS  = 0x01
        };

        enum class thermal_reg_enum // for BQ25601_REG05, TREG
        {
            TREG_90C   = 0x00,
            TREG_110C  = 0x01
        };

        bool get_enable_termination();
        void set_enable_termination(bool value);
        watchdog_enum get_watchdog_time();
        void set_watchdog_time(watchdog_enum value);
        
        bool get_enable_timer();
        void set_enable_timer(bool value);
        charge_time_enum get_charge_timer();
        void set_charge_timer(charge_time_enum value);
        thermal_reg_enum get_thermal_reg_temp();
        void set_thermal_reg_temp(thermal_reg_enum value);

        //Reg 06
        enum class ovp_enum  // for BQ25601_REG06, OVP
        {
            OVP_5_5V    = 0x00,
            OVP_6_5V    = 0x01,
            OVP_10_5V   = 0x02,
            OVP_14V     = 0x03
        };

        enum class boost_voltage_enum // for BQ25601_REG06, BOOSTV
        {
            BOOSTV_4_85V    = 0x00,
            BOOSTV_5_00V    = 0x01,
            BOOSTV_5_15V    = 0x02,
            BOOSTV_5_30V    = 0x03
        };

        ovp_enum get_ovp();
        void set_ovp(ovp_enum value);
        boost_voltage_enum  get_boost_voltage();
        void set_boost_voltage(boost_voltage_enum value);
        uint16_t get_absolute_vindpm_threshold_mV();
        uint16_t set_absolute_vindpm_threshold_mV(uint16_t mV);

        // Reg 07
        enum class vdpm_bat_track_enum // for BQ25601_REG07, VDPM_BAT_TRACK
        {
            TRACK_DISABLE  = 0x00,
            TRACK_200mV    = 0x01,
            TRACK_250mV    = 0x02,
            TRACK_300mV    = 0x03
        };

        bool get_in_current_lim_detection();
        void set_in_current_lim_detection(bool value);
        bool get_x2_slow_safety_timer();
        void set_x2_slow_safety_timer(bool value);
        bool get_batfet_disable();
        void set_batfet_disable(bool value);
        bool get_batfet_delay();
        void set_batfet_delay(bool value);
        bool get_batfet_reset_enable();
        void set_batfet_reset_enable(bool value);
        vdpm_bat_track_enum get_vdpm_bat_track();
        void set_vdpm_bat_track(vdpm_bat_track_enum value);

        // Reg 08 
        enum class vbus_status_enum  // for BQ25601_REG08, VBUS_STAT
        {
            NO_INPUT        = 0x00, // No input
            USB_HOST_SDP    = 0x01, // USB Host SDP (500 mA) → PSEL HIGH
            USB_CDP         = 0x02, // 1.5A,
            USB_DSC         = 0x03, // 2.4A
            UNKNOWN_ADAPTER = 0x05, // 500 mA
            NON_STANDARD    = 0x06, // Non-Standard Adapter (1A/2A/2.1A/2.4A)
            OTG             = 0x07  // OTG
        };
        
        enum class charge_status_enum // for BQ25601_REG08, CHRG_STAT
        {
            NOT_CHARGING    = 0x00, // Not Charging
            PRE_CHARGE      = 0x01, // Pre-charge (< VBATLOWV)
            FAST_CHARGING   = 0x02, // Fast Charging
            CHARGE_TERM     = 0x03  // Charge Termination
        };

        vbus_status_enum vbus_source();
        charge_status_enum charge_status();
        bool power_good();
        bool thermal_status();
        bool vsys_reg();

        // Reg 09
        enum class charge_fault_enum  // for BQ25601_REG09, CHRG_FAULT
        {
            NORMAL      = 0x00, // Normal
            INPUT_FAULT = 0x01, // Input fault (VAC OVP or VBAT < VBUS < 3.8 V)
            THERM_SHDN  = 0x02, // Thermal shutdown
            TIMER_EXP   = 0x03  // Charge Safety Timer Expiration
        };
        
        enum class ntc_fault_enum  // for BQ25601_REG09, NTC_FAULT
        {
            NORMAL      = 0x00, //  
            WARM        = 0x02, // buck mode only
            COOL        = 0x03, // buck mode only
            COLD        = 0x05, //
            HOT         = 0X06  // 
        };

        bool watchdog_fault();
        bool boost_fault();
        charge_fault_enum charge_fault();
        bool bat_fault();
        ntc_fault_enum ntc_fault();

        // Reg 0A
        bool vbus_power_good();
        bool in_vindpm();
        bool in_iindpm();
        bool top_off_active();
        bool input_over_voltage();
        bool get_mask_vindpm_int();
        void set_mask_vindpm_int(bool value);
        bool get_mask_iindpm_int();
        void set_mask_iindpm_int(bool value);

        // Reg 0B
        enum class pn_enum // for BQ25601_REG0B, PN
        {
            BQ25601D     = 0x02
        };
 
        bool get_register_reset();
        void set_register_reset(bool value);
        pn_enum pn();

    private:
        void set_register(uint8_t reg, uint8_t value);
        uint16_t get_vreg_mV_from_reg(uint8_t reg_value);
        uint8_t _register[BQ25601_REG_MAX+1] = {0};

 };
 
 #endif // _BQ25601_H
