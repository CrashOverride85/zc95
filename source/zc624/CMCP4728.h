#ifndef _CCMCP4728_H
#define _CCMCP4728_H

#include "CDac.h"
#include "hardware/i2c.h"
#include <inttypes.h>

#define MCP4728_MULTI_IR_CMD     0x40 

class CMCP4728 : public CDac
{
    public:
        CMCP4728(i2c_inst_t *i2c, uint8_t address);    
        void set_channel_value(dac_channel channel, uint16_t value);

    private:
        i2c_inst_t *_i2c;
        uint8_t _address;

};

#endif
