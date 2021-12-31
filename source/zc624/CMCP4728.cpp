
#include "CMCP4728.h"
#include "stdio.h"

// based on https://github.com/adafruit/Adafruit_MCP4728/blob/master/Adafruit_MCP4728.cpp

CMCP4728::CMCP4728(i2c_inst_t *i2c, uint8_t address)
{
    _i2c = i2c;
    _address = address;
}

// Set channel value - value must be 0-4000
void CMCP4728::set_channel_value(dac_channel channel, uint16_t value)
{
    uint8_t output_buffer[3] = {0};

    uint8_t sequential_write_cmd = MCP4728_MULTI_IR_CMD;

    if (value > 4095)
        value = 4095;

    sequential_write_cmd |= ( ((uint8_t)channel << 1));

    output_buffer[0] = sequential_write_cmd;
    output_buffer[1] = (value >> 8) & 0x0F;
    output_buffer[2] = value & 0xFF;

    int bytes_written = i2c_write_timeout_us(_i2c, _address, output_buffer, sizeof(output_buffer), false, 2000);
    if (bytes_written != sizeof(output_buffer))
    {
        printf("CMCP4728::write failed! i2c bytes_written = %d\n", bytes_written);
    }

    return;
}










