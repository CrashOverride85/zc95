#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "CI2cSlave.h"
#include "../common/zc624_config.h"
#include "../common/FirmwareMagicNumber.h"
#include "git_version.h"

static struct
{
    // Note: size of 256 & uint8_t for address is using wrap around to avoid out of bounds reads, so
    //       increasing mem size and changing mem_address size without adding protection would be bad
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

CI2cSlave::~CI2cSlave()
{
    i2c_slave_deinit(I2C_PORT_SLAVE);
}

void CI2cSlave::set_value(uint8_t reg, uint8_t value)
{
   i2c_slave_context.mem[(uint8_t)reg] = value;
}

uint8_t CI2cSlave::get_value(CI2cSlave::reg reg)
{
   return i2c_slave_context.mem[(uint8_t)reg];
}

void CI2cSlave::init_with_default_values()
{
    // wipe i2c registers
    for (uint8_t x=0; x < 0xFF; x++)
    {
        i2c_slave_context.mem[x] = 0;
    }

    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::TypeLow]          = DEVICE_TYPE &  0xFF;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::TypeHigh]         = (DEVICE_TYPE >> 8) & 0xFF;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::VersionMajor]     = VERSION_MAJOR;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::VersionMinor]     = VERSION_MINOR;

    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::OverallStatus]    = ZC624_OVERALL_STATUS_STARTUP;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan0Status]      = ZC624_OVERALL_STATUS_STARTUP;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan1Status]      = ZC624_OVERALL_STATUS_STARTUP;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan2Status]      = ZC624_OVERALL_STATUS_STARTUP;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan3Status]      = ZC624_OVERALL_STATUS_STARTUP;
    
    // Read/Write registers
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::ChannelIsolation] = true;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Bootloader]       = ZC624_REG_BOOTLOADER_STATE_RUN_MAIN_FIRMWARE;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::EraseFirmware]    = ZC624_REG_ERASEFW_NA;

    // Copy version into i2c_slave_context.mem
    uint version_len = strlen(firmware_info.firmware_version);
    uint version_space = (uint8_t)CI2cSlave::reg::VerStrEnd - (uint8_t)CI2cSlave::reg::VerStrStart;
    if (version_len > version_space)
        version_len = version_space;
    for (uint8_t x=0; x < version_len; x++)
        i2c_slave_context.mem[((uint8_t)CI2cSlave::reg::VerStrStart) + x] = firmware_info.firmware_version[x];

    // Copy bootloader version into i2c_slave_context.mem
    const char* bl_version = get_zc624_bootloader_version().c_str();
    version_len = strlen(bl_version);
    version_space = (uint8_t)CI2cSlave::reg::BlVerStrEnd - (uint8_t)CI2cSlave::reg::BlVerStrStart;
    if (version_len > version_space)
        version_len = version_space;
    for (uint8_t x=0; x < version_len; x++)
        i2c_slave_context.mem[((uint8_t)CI2cSlave::reg::BlVerStrStart) + x] = bl_version[x];
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
            i2c_slave_context.mem_address++;
            break;

        case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
            i2c_slave_context.mem_address_written = false;
            break;

        default:
            break;
    }
}

std::string CI2cSlave::get_zc624_bootloader_version()
{
    firmware_info_t fw_info;

    // For the bootloader, the version block is in the last 128 bytes of it.
    // PROGRAM_OFFSET here is start of the main f/w.
    memcpy(&fw_info, (void*)(XIP_BASE + PROGRAM_OFFSET - 128), sizeof(firmware_info_t));
    if (fw_info.magic != FIRMWARE_VERSION_624BL_MAGIC)
    {
        strcpy(fw_info.firmware_version, "<invalid>");
    }

    return fw_info.firmware_version;
}
