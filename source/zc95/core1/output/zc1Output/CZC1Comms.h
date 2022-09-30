#ifndef _CZC1COMMS_H
#define _CZC1COMMS_H

#include <stdint.h>
#include <string>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"

class CZC1Comms
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
            Pulse = 0,
            SetPower = 1,
            Poll = 2,
            PowerDown = 3,
            
            SetFreq = 4,
            SetPulseWitdh = 5,
            SwitchOn = 6,
            SwitchOff = 7
        };

        enum class i2c_reg_t
        {
            // Read only
            TypeLow          = 0x00,
            TypeHigh         = 0x01,
            VersionMajor     = 0x02,
            VersionMinor     = 0x03,

            OverallStatus    = 0x0F,
            Chan0Status      = 0x10,
            Chan1Status      = 0x11,
            Chan2Status      = 0x12,
            Chan3Status      = 0x13,

            VerStrStart      = 0x20,
            VerStrEnd        = 0x34, //  20 character string

            // Read/write
            // starting at 0x80
            ChannelIsolation = 0x80
        };

        enum status
        {
            Startup   = 0x00,
            Ready     = 0x01,
            Fault     = 0x02
        };

        bool check_zc624();
        std::string get_version();
        bool get_major_minor_version(uint8_t *major, uint8_t *minor);
        
        CZC1Comms(spi_inst_t *spi, i2c_inst_t *i2c);
        ~CZC1Comms();

        void send_message(message msg);
        bool write_i2c_register(i2c_reg_t reg, uint8_t value);
        bool get_i2c_register(i2c_reg_t reg, uint8_t *value);

    private:
        bool get_i2c_register_range(i2c_reg_t reg, uint8_t *buffer, uint8_t size);

        spi_inst_t *_spi;
        i2c_inst_t *_i2c;
};


#endif
