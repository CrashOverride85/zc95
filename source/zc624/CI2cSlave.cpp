#include "pico/stdlib.h"
#include <stdio.h>

#include "CI2cSlave.h"
#include "config.h"

static struct
{
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
} i2c_slave_context;

 CI2cSlave::CI2cSlave()
 {
    printf("CI2cSlave()\n");
    init_with_default_values();

    // I2C Initialisation as slave (connected to main board). Using it at 400Khz.
    gpio_init(I2C_SDA_SLAVE);
    gpio_set_function(I2C_SDA_SLAVE, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_SLAVE);

    gpio_init(I2C_SCL_SLAVE);
    gpio_set_function(I2C_SCL_SLAVE, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_SLAVE);

    i2c_init(I2C_PORT_SLAVE, 400*1000);
    i2c_slave_init(I2C_PORT_SLAVE, ZC624_ADDR, &CI2cSlave::i2c_slave_handler);
 }

 void CI2cSlave::set_value(uint8_t reg, uint8_t value)
 {
    i2c_slave_context.mem[(uint8_t)reg] = value;
 }

void CI2cSlave::init_with_default_values()
{
    // wipe i2c registers
    for (uint8_t x=0; x < 0xFF; x++)
    {
        i2c_slave_context.mem[x] = 0;
    }

    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::TypeLow]        = DEVICE_TYPE &  0xFF;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::TypeHigh]       = (DEVICE_TYPE >> 8) & 0xFF;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::VersionMajor]   = VERSION_MAJOR;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::VersionMinor]   = VERSION_MINOR;

    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::OverallStatus]  = CI2cSlave::status::Startup;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan0Status]    = CI2cSlave::status::Startup;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan1Status]    = CI2cSlave::status::Startup;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan2Status]    = CI2cSlave::status::Startup;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan3Status]    = CI2cSlave::status::Startup;

}

void CI2cSlave::i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    switch (event)
    {
        case I2C_SLAVE_RECEIVE: // master has written some data
            if (!i2c_slave_context.mem_address_written) 
            {
                // writes always start with the memory address
                i2c_slave_context.mem_address = i2c_read_byte(i2c);
                i2c_slave_context.mem_address_written = true;
            } 
            else 
            {
                if (i2c_slave_context.mem_address >= 0x80) // below 0x80 (128) is read only
                {
                    // save into memory
                    i2c_slave_context.mem[i2c_slave_context.mem_address] = i2c_read_byte(i2c);
                }
            }
            break;

        case I2C_SLAVE_REQUEST: // master is requesting data
            // load from memory
            i2c_write_byte(i2c, i2c_slave_context.mem[i2c_slave_context.mem_address]);
            break;

        case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
            i2c_slave_context.mem_address_written = false;
            break;

        default:
            break;
    }
}

