#ifndef _CHWCHECKZC624_H
#define _CHWCHECKZC624_H

#include <inttypes.h>
#include <string>
#include "hardware/i2c.h"

class CHwCheckZc624
{
    public:
        enum class reg
        {
            // Read only
            TypeLow        = 0x00,
            TypeHigh       = 0x01,
            VersionMajor   = 0x02,
            VersionMinor   = 0x03,

            OverallStatus  = 0x0F,
            Chan0Status    = 0x10,
            Chan1Status    = 0x11,
            Chan2Status    = 0x12,
            Chan3Status    = 0x13,

            VerStrStart    = 0x20,
            VerStrEnd      = 0x34 //  20 character string

            // Read/write
            // starting at 0x80
        };

        enum status
        {
            Startup   = 0x00,
            Ready     = 0x01,
            Fault     = 0x02
        };

        bool check_zc624();
        std::string get_version();

    private:
        bool get_i2c_register(uint8_t address, uint8_t reg, uint8_t *value);
        bool get_i2c_register_range(uint8_t address, uint8_t reg, uint8_t *buffer, uint8_t size);
};

#endif
