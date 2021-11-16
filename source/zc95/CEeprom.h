#ifndef _CEEPROM_H
#define _CEEPROM_H


#include "hardware/i2c.h"
#include <inttypes.h>

class CEeprom
{
    public:
        CEeprom(i2c_inst_t *i2c, uint8_t i2c_address);
        uint8_t read(uint16_t address);
        bool write(uint16_t address, uint8_t value, bool block_until_complete);

    private:
        uint8_t _i2c_address;
        i2c_inst_t *_i2c;
};

#endif
