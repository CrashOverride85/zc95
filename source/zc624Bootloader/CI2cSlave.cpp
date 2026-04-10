#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "CI2cSlave.h"
#include "../common/zc624_config.h"
#include "git_version.h"

uint16_t CI2cSlave::_s_fw_block_count;

static struct
{
    // Note: size of 256 & uint8_t for address is using wrap around to avoid out of bounds reads, so
    //       increasing mem size and changing mem_address size without adding protection would be bad
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
    bool receiving_data_block;
    bool sending_data_block;
    uint8_t data_block[1 + 128 + 1]; // 1 byte1 for block number + 128 bytes of payload data + 1 byte checksum
    uint8_t data_block_idx;
    uint16_t data_block_read_idx;
    CI2cSlave::received_datablock_cb* process_datablock = NULL;
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

    set_fw_block_count(); // This is SLOW (> 100ms)
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Bootloader] = ZC624_REG_BOOTLOADER_STATE_IN_BOOTLOADER;
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

void CI2cSlave::set_datablock_callback(received_datablock_cb* cb)
{
    i2c_slave_context.process_datablock = cb;
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
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::VersionMajor]     = 0xFF; // these two get set in zc624Bootloader.cpp
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::VersionMinor]     = 99;

    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::OverallStatus]    = ZC624_OVERALL_STATUS_STARTUP;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan0Status]      = ZC624_OVERALL_STATUS_STARTUP;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan1Status]      = ZC624_OVERALL_STATUS_STARTUP;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan2Status]      = ZC624_OVERALL_STATUS_STARTUP;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Chan3Status]      = ZC624_OVERALL_STATUS_STARTUP;
    
    // Read/Write registers
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::ChannelIsolation] = true;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::Bootloader]       = ZC624_REG_BOOTLOADER_STATE_INIT;
    i2c_slave_context.mem[(uint8_t)CI2cSlave::reg::EraseFirmware]    = ZC624_REG_ERASEFW_ERASABLE;
}

void CI2cSlave::i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
{
    switch (event)
    {
        case I2C_SLAVE_RECEIVE: // master has written some data
            if (!i2c_slave_context.mem_address_written) 
            {
                // writes always start with the memory address / command
                i2c_slave_context.mem_address = i2c_read_byte(i2c);
                i2c_slave_context.mem_address_written = true;
                
                if (i2c_slave_context.mem_address == (uint8_t)CI2cSlave::reg::DataBlockWrite)
                {
                    i2c_slave_context.receiving_data_block = true;
                    i2c_slave_context.sending_data_block = false;
                    i2c_slave_context.data_block_idx = 0;
                    memset(i2c_slave_context.data_block, 0, sizeof(i2c_slave_context.data_block));

                    for (size_t n = i2c_get_read_available(i2c); n > 0; n--) 
                    {
                        uint8_t rx_byte = i2c_read_byte(i2c);
                        if (i2c_slave_context.data_block_idx < sizeof(i2c_slave_context.data_block))
                            i2c_slave_context.data_block[i2c_slave_context.data_block_idx++] = rx_byte;
                    }
                }

                else if (i2c_slave_context.mem_address == (uint8_t)CI2cSlave::reg::DataBlockRead)
                {
                    i2c_slave_context.receiving_data_block = false;
                    i2c_slave_context.sending_data_block = true;

                    uint8_t blk[2];
                    i2c_read_raw_blocking(I2C_PORT_SLAVE, blk, sizeof(blk));
                    i2c_slave_context.data_block_read_idx = blk[0];
                    i2c_slave_context.data_block_read_idx |= blk[1] << 8;
                }

                else
                {
                    i2c_slave_context.sending_data_block = false;
                }
            }

            else if (i2c_slave_context.receiving_data_block)
            {
                for (size_t n = i2c_get_read_available(i2c); n > 0; n--) 
                {
                    uint8_t rx_byte = i2c_read_byte(i2c);
                    if (i2c_slave_context.data_block_idx < sizeof(i2c_slave_context.data_block))
                        i2c_slave_context.data_block[i2c_slave_context.data_block_idx++] = rx_byte;
                }
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
            if (i2c_slave_context.sending_data_block)
            {
                // Master is requesting a flash block
                uint8_t* flash_ptr = (uint8_t*)(XIP_BASE + PROGRAM_OFFSET + (i2c_slave_context.data_block_read_idx * 128));
                i2c_write_raw_blocking(I2C_PORT_SLAVE, flash_ptr, 128);
            }

            else if (i2c_slave_context.mem_address == (uint8_t)CI2cSlave::reg::FwBlockCount)
            {
                uint8_t blk[2];
                blk[0] = _s_fw_block_count & 0xFF;
                blk[1] = (_s_fw_block_count >> 8) & 0xFF;
                i2c_write_raw_blocking(I2C_PORT_SLAVE, blk, 2);
            }
            else
            {
                // load from memory
                i2c_write_byte(i2c, i2c_slave_context.mem[i2c_slave_context.mem_address]);
                i2c_slave_context.mem_address++;
            }
            break;

        case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
            i2c_slave_context.mem_address_written = false;
            if (i2c_slave_context.receiving_data_block)
            {
                if (i2c_slave_context.process_datablock != NULL)
                {
                    i2c_slave_context.process_datablock(i2c_slave_context.data_block, i2c_slave_context.data_block_idx);
                }
                
                i2c_slave_context.receiving_data_block = false;
            }
            break;

        default:
            break;
    }
}

// Figure out how big the main firmware in flash is. Store the result in 
// _s_fw_block_count as <size in bytes> / 128,
// Work the size out by starting at the end of flash, and going backwards 
// until the first byte that isn't 0xFF is found.
void CI2cSlave::set_fw_block_count()
{
    uint8_t* flash_ptr = (uint8_t*)(XIP_BASE + PICO_FLASH_SIZE_BYTES);
    while (*--flash_ptr == 0xFF)
        ;

    // flash_ptr should now point to the end of the firmware
    uint32_t fw_size = ((uint32_t)flash_ptr - XIP_BASE + 1) - PROGRAM_OFFSET; // -PROGRAM_OFFSET to not include the bootloader in the count
    
    _s_fw_block_count = (fw_size / 128) + 1;
    printf("Firmware size: %d blocks (%d bytes)\n", _s_fw_block_count, fw_size);
}
