

#ifndef _CI2CSLAVE_H
#define _CI2CSLAVE_H

#include <inttypes.h>

#include <i2c_fifo.h>
#include <i2c_slave.h>



class CI2cSlave
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
            VerStrEnd      = 0x34, //  20 character string

            // Read/write
            // starting at 0x80
            ChannelIsolation = 0x80 // Default = true. If false, multiple channels can pulse at the same time (triphase effects)
        };

        enum status
        {
            Startup   = 0x00,
            Ready     = 0x01,
            Fault     = 0x02
        };

        CI2cSlave();
        void set_value(uint8_t reg, uint8_t value);
        uint8_t get_value(CI2cSlave::reg reg);


    private:
        static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event);
        void init_with_default_values();
};

#endif


