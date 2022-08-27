#include "CMCP4651.h"
#include "hardware/i2c.h"
#include <stdio.h>

/* 
 * Set the value on the dual MCP4651 digital potentiometer
 * Used to control gain of opamp. Lower value = higher gain.
 */


CMCP4651::CMCP4651(uint8_t addr)
{
    _address = addr;
}

void CMCP4651::set_val(uint8_t pot, uint16_t val)
{
    uint8_t buffer[2];

    buffer[0] = (pot & 3) << 4 | ((val >> 8) & 3);
    buffer[1] = val & 0xFF;

    int retval = i2c_write_timeout_us(i2c_default, _address, buffer, sizeof(buffer), false, 1000);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CMCP4651::set_val i2c write error! (%d)\n", retval);
      return;
    }
}
