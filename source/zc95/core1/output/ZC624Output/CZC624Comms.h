#ifndef _CZC624COMMS_H
#define _CZC624COMMS_H

#include <stdint.h>
#include <string>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "../../../common/i2cEnums.h"

class CZC624Comms
{
    public:
        struct message
        {
            uint8_t command;
            uint8_t arg0;
            uint8_t arg1;
            uint8_t arg2;
        };

        enum class spi_command_t
        {
            SetPower = 1,
            Poll = 2,
            PowerDown = 3, // If renumbering, also change in globals.cpp
            
            SetFreq = 4,
            SetPulseWidth = 5,
            SwitchOn = 6,
            SwitchOff = 7,
            NoOp = 8,
            SetTestVal = 9,
            Pulse = 10,
            SyncChanel = 11
        };

        enum class i2c_reg_t
        {
            // Read only
            TypeLow          = ZC624_REG_TYPELOW,
            TypeHigh         = ZC624_REG_TYPEHIGH,
            VersionMajor     = ZC624_REG_VERSIONMAJOR,
            VersionMinor     = ZC624_REG_VERSIONMINOR,

            OverallStatus    = ZC624_REG_OVERALLSTATUS,
            Chan0Status      = ZC624_REG_CHAN0STATUS,
            Chan1Status      = ZC624_REG_CHAN1STATUS,
            Chan2Status      = ZC624_REG_CHAN2STATUS,
            Chan3Status      = ZC624_REG_CHAN3STATUS,

            VerStrStart      = ZC624_REG_VERSTRSTART,
            VerStrEnd        = ZC624_REG_VERSTREND, //  20 character string

            TestVal          = ZC624_REG_TESTVAL,

            BlVerStrStart    = ZC624_REG_BL_VERSTRSTART, // Bootloader version
            BlVerStrEnd      = ZC624_REG_BL_VERSTREND,   // 20 character string

            // Read/write
            // starting at 0x80
            ChannelIsolation = ZC624_REG_CHANNELISOLATION,
            Bootloader       = ZC624_REG_BOOTLOADER,
            DataBlockWrite   = ZC624_REG_DATABLOCKWRITE,
            EraseFirmware    = ZC624_REG_ERASEFIRMWARE
        };

        enum status
        {
            Startup       = ZC624_OVERALL_STATUS_STARTUP,
            Ready         = ZC624_OVERALL_STATUS_READY,
            Fault         = ZC624_OVERALL_STATUS_FAULT,
            FirmwareFault = ZC624_OVERALL_STATUS_FAULTFIRMWARE
        };

        uint8_t check_zc624();
        std::string get_version(bool bootloader);
        bool get_major_minor_version(uint8_t *major, uint8_t *minor);
        bool spi_has_comms_fault();
        
        CZC624Comms(spi_inst_t *spi, i2c_inst_t *i2c);
        ~CZC624Comms();

        void send_message(message msg);
        bool write_i2c_register(i2c_reg_t reg, uint8_t value);
        bool get_i2c_register(i2c_reg_t reg, uint8_t *value);
        bool exit_bootloader();
        bool has_started_main_firmware();
        bool loop(uint8_t channel_id);

    private:
        bool get_i2c_register_range(i2c_reg_t reg, uint8_t *buffer, uint8_t size);
        bool channel_has_fault(uint8_t channel);
        std::string status_to_string(status s);
        bool test_spi_comms(uint8_t test_val);

        spi_inst_t *_spi;
        i2c_inst_t *_i2c;
        uint8_t _led_state = 0;
        uint64_t _last_msg_us = 0;
};


#endif
