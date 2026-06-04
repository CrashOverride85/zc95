#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/flash.h"
#include "CI2cSlave.h"

#include "pico/stdlib.h"
#include "i2c.h"
#include "git_version.h"

#include "../common/zc624_config.h"
#include "../common/FirmwareMagicNumber.h"

#define BOOTLOADER_SIZE_K 48
#define PROGRAM_OFFSET (BOOTLOADER_SIZE_K * 1024) 

uint8_t data_received_buffer[1 + 128 + 1]; // block number + payload + checksum
volatile bool data_received = false;
volatile bool end_of_transmission = false;
static uint8_t _current_block = 1;

CI2cSlave* _i2c_slave = NULL;

void get_flash_firmware_version(firmware_info_t* fw_info)
{
    memcpy(fw_info, (const void*)(XIP_BASE + PROGRAM_OFFSET), sizeof(firmware_info_t));
    if (fw_info->magic != FIRMWARE_VERSION_624_MAGIC)
    {
        strcpy(fw_info->firmware_version, "<invalid>");
        fw_info->fw624_major = 0xFF;
        fw_info->fw624_minor = 0x00;
    }
}

bool is_firmware_in_flash_valid()
{
    firmware_info_t fw_info_flash;
    get_flash_firmware_version(&fw_info_flash);

    return (fw_info_flash.magic == FIRMWARE_VERSION_624_MAGIC);
}

// Taken from:
//   https://vanhunteradams.com/Pico/Bootloader/Bootloader.html
// Set VTOR register, set stack pointer, and jump to the reset
// vector in our application. Basically copied from crt0.S.
static inline void launch_firmware() 
{
    if (!is_firmware_in_flash_valid())
    {
        printf("Attempt to launch invalid firmware!\n");
        panic("Invalid firmware\n");
    }

    if (_i2c_slave != NULL)
    {
        delete _i2c_slave;
        _i2c_slave = NULL;
    }
    printf("Launching firmware\n");

    // In an assembly snippet . . .
    // Set VTOR register, set stack pointer, and jump to reset
    asm volatile (
    "mov r0, %[start]\n"
    "ldr r1, =%[vtable]\n"
    "str r0, [r1]\n"
    "ldmia r0, {r0, r1}\n"
    "msr msp, r0\n"
    "bx r1\n"
    :
    : [start] "r" (XIP_BASE + PROGRAM_OFFSET + 128), [vtable] "X" (PPB_BASE + M0PLUS_VTOR_OFFSET)
    :                                     // + 128 to skip over f/w info block
    );
}

static void call_flash_range_erase(void *param)
{
    flash_range_erase(PROGRAM_OFFSET, PICO_FLASH_SIZE_BYTES - PROGRAM_OFFSET);
}

static void call_flash_range_program(void *param)
{
    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}

bool clear_firmware_in_flash()
{
    printf("Wiping firmware in flash... ");
    uint32_t offset = PROGRAM_OFFSET;
    int rc = flash_safe_execute(call_flash_range_erase, NULL, UINT32_MAX);
    hard_assert(rc == PICO_OK);
    printf("done\n");

    return true;
}

bool write_buffer_to_flash(uint8_t* buffer, uint32_t total_byte_counter)
{
    uint32_t offset = PROGRAM_OFFSET;

    hard_assert(total_byte_counter >= 256);
    hard_assert((total_byte_counter % 256)== 0);
    
    offset += total_byte_counter - 256;

    uintptr_t params[] = { offset, (uintptr_t)buffer};
    int rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
    hard_assert(rc == PICO_OK);
    return true;
}

bool write_received_data_to_flash(const uint8_t* data, size_t length)
{
    static uint32_t total_byte_counter = 0;
    static uint8_t buffer[256] = {0};
    static uint8_t buffer_pos = 0;

    // Don't write the first block until after we reach the end of transmission. This should stop a 
    // firmware that was mostly written to flash from starting, then randomly crashing.
    static bool first_block = true;
    static uint8_t first_block_contents[256] = {0}; 

    if (data == NULL && length == 0)
    {
        // end of transmission. Write remaining buffer to flash.
        if (buffer_pos > 0)
        {
            if (total_byte_counter % 256 != 0)
                total_byte_counter += 128; // we must write a multiple of 256 bytes to flash, so just always write exactly 256

            write_buffer_to_flash(buffer, total_byte_counter);
        }

        // finally write the first block
        write_buffer_to_flash(first_block_contents, sizeof(first_block_contents));
        printf("End of transmission\n");

        return true;
    }

    if (data != NULL && total_byte_counter == 0)
    {
        first_block = true;
    }

    for (size_t x = 0; x < length; x++)
    {
        total_byte_counter++;
        buffer[buffer_pos] = data[x];
        if (buffer_pos == 255)
        {
            if (first_block)
            {
                memcpy(first_block_contents, buffer, sizeof(first_block_contents));
                first_block = false;
            }
            else
                write_buffer_to_flash(buffer, total_byte_counter);

            memset(buffer, 0, 256);
        }
        buffer_pos++;
    }

    return true;
}


bool is_data_buffer_valid(const uint8_t* data, size_t length)
{
    if (length != sizeof(data_received_buffer))
    {
        printf("error: unexpected data length - expected %d got %d\n", sizeof(data_received_buffer), length);
        return false;
    }

    uint8_t checksum = 0;
    for (int i=0; i < 128; i++)
        checksum += data[i+1]; // skip over block number

    if (checksum != data[sizeof(data_received_buffer)-1])
    {
        printf("invalid checksum, calculated %d got %d\n", checksum, data[sizeof(data_received_buffer)-1]);
        return false;
    }

    if (data[0] != _current_block)
    {
        printf("error: expected block %d got block %d\n", _current_block, data[0]);
        return false;
    }

    printf("Got block %d\n", _current_block);

    _current_block++;

    return true;
}

// This is called from an ISR, so can't do a lot
void i2c_datablock_received(const uint8_t* data, size_t length)
{
    if (length > sizeof(data_received_buffer))
    {
        printf("datablock too large. Got %d, expected %d\n", length, sizeof(data_received));
        return;
    }

    // look for an end of transmission message
    if (length == 1 && data[0]==0)
    {
        end_of_transmission = true;
    }

    else if (is_data_buffer_valid(data, length))
    {
        memcpy(data_received_buffer, data, length);
        data_received = true;
    }
}

void init_i2c_version_registers()
{
    // Set main firmware version
    firmware_info_t main_fw_version;
    get_flash_firmware_version(&main_fw_version);
    uint version_len = strlen(main_fw_version.firmware_version);
    uint version_space = (uint8_t)CI2cSlave::reg::VerStrEnd - (uint8_t)CI2cSlave::reg::VerStrStart;
    if (version_len > version_space)
        version_len = version_space;
    for (uint8_t x=0; x < version_len; x++)
        _i2c_slave->set_value(((uint8_t)CI2cSlave::reg::VerStrStart) + x, main_fw_version.firmware_version[x]);

    _i2c_slave->set_value(((uint8_t)CI2cSlave::reg::VersionMajor), main_fw_version.fw624_major);
    _i2c_slave->set_value(((uint8_t)CI2cSlave::reg::VersionMinor), main_fw_version.fw624_minor);

    printf("Main firmware version: major=%d, minor=%d : [%s]\n", 
        main_fw_version.fw624_major, main_fw_version.fw624_minor, main_fw_version.firmware_version);

    // Set bootloader version
    version_len = strlen(firmware_info.firmware_version);
    version_space = (uint8_t)CI2cSlave::reg::BlVerStrEnd - (uint8_t)CI2cSlave::reg::BlVerStrStart;
    if (version_len > version_space)
        version_len = version_space;
    for (uint8_t x=0; x < version_len; x++)
        _i2c_slave->set_value(((uint8_t)CI2cSlave::reg::BlVerStrStart) + x, firmware_info.firmware_version[x]);
}

void flash_ok_led(uint8_t freq_hz)
{
    static uint32_t last_change_time_us = 0;
    static bool led_state = 0;
    uint32_t interval_us = 1000000 / freq_hz;

    if (time_us_32() - last_change_time_us > interval_us)
    {
        led_state = !led_state;
        gpio_put(PIN_OK_LED, led_state);
        last_change_time_us = time_us_32();
    }
}

int main()
{
    stdio_init_all();
    _i2c_slave = new CI2cSlave();
    printf("\nZC624 bootloader version %s\n", firmware_info.firmware_version);
    init_i2c_version_registers();
    _i2c_slave->set_datablock_callback(&i2c_datablock_received);
   
    gpio_init(PIN_OK_LED);
    gpio_set_dir(PIN_OK_LED, GPIO_OUT);
    gpio_put(PIN_OK_LED, 0);

    while (true)
    {
        // If the erase firmware register has been set, erase the firmware then reset the register
        if (_i2c_slave->get_value(CI2cSlave::reg::EraseFirmware) == ZC624_REG_ERASEFW_ERASE)
        {
            clear_firmware_in_flash();
            data_received = false;
            end_of_transmission = false;

            _current_block = 1;
            _i2c_slave->set_value((uint8_t)CI2cSlave::reg::EraseFirmware, ZC624_REG_ERASEFW_ERASABLE);
        }

        if (data_received)
        {
            // write data to flash, skipping over the block number at the start, and ignoring the checksum at the end
            write_received_data_to_flash(data_received_buffer+1, sizeof(data_received_buffer)-2);
            data_received = false;
        }

        if (end_of_transmission)
        {
            write_received_data_to_flash(NULL, 0);
            end_of_transmission = false;
        }

        if (_i2c_slave->get_value(CI2cSlave::reg::Bootloader) == ZC624_REG_BOOTLOADER_STATE_RUN_MAIN_FIRMWARE)
        {
            if (is_firmware_in_flash_valid())
            {
                launch_firmware(); // should never return
                while(1);
            }

            printf("ERROR: unable to launch main firmware as not valid\n");
            _i2c_slave->set_value((uint8_t)CI2cSlave::reg::Bootloader, ZC624_REG_BOOTLOADER_STATE_IN_BOOTLOADER);
        }

        flash_ok_led(10);
    }    
}
