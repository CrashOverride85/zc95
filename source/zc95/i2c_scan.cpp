#include "i2c_scan.h"
#include "config.h"
#include <stdio.h>
#include "pico/stdlib.h"

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool i2c_scan::reserved_addr(uint8_t addr) 
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_scan::scan(i2c_inst_t *i2c)
{
    for (int addr = 0; addr < (1 << 7); ++addr) 
    {
        if (addr % 16 == 0) 
        {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
        {
            if (addr == FP_0_2_PORT_EXP_ADDR)
            {
                // The TCA9534 I/O expander used on the front panel is weird in that it 
                // ignores reads unless there's a write first.
                // So we need a special case to detect it.
                rxdata = 0;
                ret = i2c_write_blocking(i2c, addr, &rxdata, 1, false);
            }
            
            ret = i2c_read_blocking(i2c, addr, &rxdata, 1, false);
        }

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
}
