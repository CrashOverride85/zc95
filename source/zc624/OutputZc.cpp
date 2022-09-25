#include "config.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"

#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/spi.h"

#include "CMCP4728.h"
#include "COutput.h"
#include "CMessageProcess.h"
#include "messages.h"
#include "CI2cSlave.h"
#include "git_version.h"

/*
 * Core0 handles incoming SPI messages, and output/generation (via pio) of signals to drive the FETs. Any 
 * messages received which are to change the power level are passed to core1.
 * Core1 deals with I2C comms to set power levels, as this can be a little slow (~200us per power change)
 */

void i2c_scan();
void core1_entry();

CI2cSlave *i2c_slave;

int main()
{
    stdio_init_all();
    adc_init();
    
    printf("ZC624 startup, firmware version: %s\n", kGitHash);
    i2c_slave = new CI2cSlave();

    // I2C Initialisation as master (connected to DAC). Using it at 400Khz.
    i2c_init(I2C_PORT_PER, 400*1000);
    
    gpio_set_function(I2C_SDA_PER, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PER, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PER);
    gpio_pull_up(I2C_SCL_PER);

    i2c_scan();
    multicore_reset_core1();
    multicore_launch_core1(core1_entry);

    // Control of 4x output channels
    COutput output = COutput(pio0, i2c_slave);

    // Comms to main board
    CMessageProcess spi_message_process = CMessageProcess(&output);
    spi_message_process.init();
    uint64_t loop_start = time_us_64();
    uint64_t readable=0;
    uint64_t unreadable=0;

    while(1)
    {
        if (spi_is_readable(spi0))
        {
            spi_message_process.loop();
            readable++;
        }
        else
        {
            unreadable++;
        }
        
        if (time_us_64() - loop_start > 1000000)
        {
            // printf("running... (readable=%" PRIu64 ", unreadable=%" PRIu64 "\n", readable, unreadable);
            loop_start = time_us_64();
            readable=0;
            unreadable=0;
        }

        output.loop();

        if (i2c_slave->get_value(CI2cSlave::reg::OverallStatus) == CI2cSlave::status::Fault)
        {
            printf("HALT.\n");
            while(1);
        } 
    }

    return 0;
}

void core1_entry() 
{
    printf("core1_entry()\n");
    CMCP4728 dac = CMCP4728(I2C_PORT_PER, DAC_ADDRESS);

    while(1)
    {
        message msg;
        msg.msg32 = multicore_fifo_pop_blocking();
        
        switch(msg.msg8[0])
        {
            case MESSAGE_SET_DAC_POWER:
            {
                uint8_t channel = msg.msg8[1];
                uint16_t value = msg.msg8[2];
                value |= msg.msg8[3] << 8;
                // printf("set power chan %d to %d\n", channel, value);
                dac.set_channel_value((CDac::dac_channel)channel, value);
            }
                break;

            default:
                printf("Unknown message type received: %d\n", msg.msg8[0]);
                break;
        }
    }
}

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr) 
{
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_scan()
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
            ret = i2c_read_blocking(I2C_PORT_PER, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
}
