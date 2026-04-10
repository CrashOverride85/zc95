

#ifndef _CI2CSLAVE_H
#define _CI2CSLAVE_H

#include <inttypes.h>

#include <i2c_fifo.h>
#include <i2c_slave.h>

#include "../common/i2cEnums.h"

class CI2cSlave
{
    public:       
        typedef void (received_datablock_cb)(const uint8_t* data, size_t length);

        enum class reg
        {
            TypeLow        = ZC624_REG_TYPELOW,
            TypeHigh       = ZC624_REG_TYPEHIGH,
            VersionMajor   = ZC624_REG_VERSIONMAJOR,
            VersionMinor   = ZC624_REG_VERSIONMINOR,

            OverallStatus  = ZC624_REG_OVERALLSTATUS,
            Chan0Status    = ZC624_REG_CHAN0STATUS,
            Chan1Status    = ZC624_REG_CHAN1STATUS,
            Chan2Status    = ZC624_REG_CHAN2STATUS,
            Chan3Status    = ZC624_REG_CHAN3STATUS,

            VerStrStart    = ZC624_REG_VERSTRSTART,
            VerStrEnd      = ZC624_REG_VERSTREND, //  20 character string

            TestVal        = ZC624_REG_TESTVAL,
            DataBlockRead  = ZC624_REG_DATABLOCKREAD,
            FwBlockCount   = ZC624_REG_FW_BLOCK_COUNT,

            BlVerStrStart  = ZC624_REG_BL_VERSTRSTART,
            BlVerStrEnd    = ZC624_REG_BL_VERSTREND, //  20 character string

            // Read/write
            // starting at 0x80
            ChannelIsolation = ZC624_REG_CHANNELISOLATION, // Default = true. If false, multiple channels can pulse at the same time (triphase effects)
            Bootloader       = ZC624_REG_BOOTLOADER,
            DataBlockWrite   = ZC624_REG_DATABLOCKWRITE,
            EraseFirmware    = ZC624_REG_ERASEFIRMWARE // write 1 to erase. Changes back to 0 once complete
        };

        CI2cSlave();
        ~CI2cSlave();
        void set_datablock_callback(received_datablock_cb* cb);
        void set_value(uint8_t reg, uint8_t value);
        uint8_t get_value(CI2cSlave::reg reg);


    private:
        static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event);
        void init_with_default_values();
        void set_fw_block_count();

        static uint16_t _s_fw_block_count;
};

#endif
