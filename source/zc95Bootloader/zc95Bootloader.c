/*
 * ZC95 Bootloader
 * Copyright (C) 2026  CrashOverride85
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 
 *
 * Bootloader for the ZC95. Needs to be built with config=MinSizeRel to fit
 * in the 48k allocated.
 * Allows for menu driven access, and API access from firmware upload utility.
 * Will also update firmware from an SD card if the appropriate EEPROM flag is
 * set - i.e. main firmware can receive a new firmware via WIFI, save it to SD
 * card, then set the flag. On next reboot it will be installed.
 * This bootloader also talks to the zc624's bootloader, and can transfer 
 * firmware to that.
 * 
 * Requires firmwares uploaded to it to be in bin format, built with FLASH ORIGIN 
 * set to "0x10000000 + 48k + 128", and the first 128 bytes being the version block.
 * More detailed description in repo.
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/flash.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/flash.h"

#include "zc_types.h"
#include "zc624.h"

#include "pico/stdlib.h"
#include "rtc.h"
#include "xmodem/xmodem.h"
#include "buffered_serial.h"
#include "sdcard/sd_card.h"
#include "leds.h"
#include "eeprom.h"
#include "i2c.h"
#include "git_version.h"
#include "zc_debug.h"
#include "zc_firmware.h"


#include "../common/zc95_config.h"
#include "../common/i2cEnums.h"
#include "../common/FirmwareMagicNumber.h"


#define MENU_INITIAL_ZC95       1
#define MENU_INITIAL_ZC624      2

#define MENU_OPT_RESTORE_ORIG   1
#define MENU_OPT_RESTORE_PREV   2
#define MENU_OPT_RESTORE_PEND   3
#define MENU_OPT_UPLOAD_PEND    4
#define MENU_OPT_UPLOAD_FLASH   5
#define MENU_OPT_BACKUP_TO_PREV 6
#define MENU_OPT_LAUNCH_FW      7
#define MENU_OPT_BACK           8

#define STX 0x02
#define ETX 0x03
#define ACK 0x06
#define SYN 0x16

struct sdcard_ctx _sd_ctx;

static bool _have_sd_card = false;

bool is_firmware_in_flash_valid()
{
    firmware_info_t fw_info_flash;
    get_flash_firmware_version(&fw_info_flash);

    return (fw_info_flash.magic == FIRMWARE_VERSION_95_MAGIC);
}

static void die_error()
{
    leds_show_update_error();
    printf("Critical error.\n");
    while(true)
        ;
}

// Mostly taken from:
//   https://vanhunteradams.com/Pico/Bootloader/Bootloader.html
// Set VTOR register, set stack pointer, and jump to the reset
// vector in our application. Basically copied from crt0.S.
static inline void launch_firmware() 
{
    if (!is_firmware_in_flash_valid())
    {
        bs_printf(&_bs_ctx, "Attempt to launch invalid firmware!\n");
        die_error();
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

void xmodem_send_function(uint8_t character, void* user)
{
    bs_write(&_bs_ctx, &character, 1);
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

void xmodem_debug_output_cb(const uint8_t* str, enum xmodem_debug_level level, void* user)
{
    if (level > XMODEM_DEBUG_DEBUG)
        printf("[xmodem] %s", str);
}

bool xmodem_data_received_write_flash(const uint8_t* data, size_t length, void* user)
{
    static uint32_t total_byte_counter = 0;
    static uint8_t buffer[256] = {0};
    static uint8_t buffer_pos = 0;

    // Don't write the first block until after we reach the end of file. This should stop a 
    // firmware that was mostly written to flash from starting, then randomly crashing.
    static bool first_block = true;
    static uint8_t first_block_contents[256] = {0}; 

    if (data == NULL && length == 0)
    {
        // end of file. Write remaining buffer to flash.
        if (buffer_pos > 0)
        {
            if (total_byte_counter % 256 != 0)
                total_byte_counter += 128; // we must write a multiple of 256 bytes to flash, so just always write exactly 256

            write_buffer_to_flash(buffer, total_byte_counter);
        }

        // finally write the first block
        write_buffer_to_flash(first_block_contents, sizeof(first_block_contents));

        return true;
    }

    if (data != NULL && total_byte_counter == 0)
    {
        clear_firmware_in_flash();
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

bool xmodem_data_received_write_sd_card(const uint8_t* data, size_t length, void* user)
{
    static uint32_t total_byte_recv_counter = 0;
    static uint8_t buffer_pos = 0;
    bootloader_xmodem_t* conf = user;

    if (data == NULL && length == 0 && conf->file_open)
    {
        // end of file.
        return sd_card_close(&_sd_ctx);
    }
    
    else if (total_byte_recv_counter == 0)
    {
        total_byte_recv_counter += length;

        // Open SD card
        //sd_card_init(&_sd_ctx);
        if (!sd_card_open(&_sd_ctx, conf->filename, true))
            return false;

        conf->file_open = true;
    }

    if (conf->file_open)
    {
        if (!sd_card_write(&_sd_ctx, data, length))
            return false; 
    }
    
    return true;
}

int download_firmware(enum fw_target_e target, const char* filename) 
{
    struct xmodem_ctx ctx;
    bootloader_xmodem_t sd_xmodem_conf;
    sd_xmodem_conf.filename = filename;
    sd_xmodem_conf.file_open = false;

    bootloader_xmodem_624_t zc624_xmodem_conf;
    zc624_xmodem_conf.block = 1;
    zc624_xmodem_conf.total_byte_counter = 0;

    if (target == FW_TARGET_FLASH)
        xmodem_init(&ctx, NULL, xmodem_send_function, xmodem_data_received_write_flash, xmodem_debug_output_cb);
    else if (target == FW_TARGET_624)
        xmodem_init(&ctx, &zc624_xmodem_conf, xmodem_send_function, zc624_xmodem_data_received_send, xmodem_debug_output_cb);
    else
        xmodem_init(&ctx, &sd_xmodem_conf, xmodem_send_function, xmodem_data_received_write_sd_card, xmodem_debug_output_cb);  

    if (!_inhibit_35mm_messages) bs_printf(&_bs_ctx, "\nStart XMODEM transfer now. CTRL+X to abort\n");
    xmodem_receive(&ctx);
    while(ctx.state != XMODEM_RECV_COMPLETE && ctx.state != XMODEM_IDLE)
    {
        xmodem_loop(&ctx);
        uint8_t buf;
        size_t count = bs_read(&_bs_ctx, &buf, 1);
        if (count == 1)
        {
            xmodem_serial_rx(&ctx, buf);
        }
    }

    if (ctx.state == XMODEM_RECV_COMPLETE)
    {
        if (!_inhibit_35mm_messages) bs_printf(&_bs_ctx, "Transfer completed\n");
        printf("Transfer completed\n");
    }
    else
    {
        if (sd_xmodem_conf.file_open)
        {
            sd_card_close(&_sd_ctx);
            sd_xmodem_conf.file_open = false;
        }

        // transfer failed. If we received some data, then the sd file / flash will now be in a weird state - wipe it.
        if (ctx.total_blocks_received > 0)
        {
            if (target == FW_TARGET_FLASH)
            {
                clear_firmware_in_flash();
            }
            else if (target == FW_TARGET_SD_CARD)
            {
                uint8_t buffer[128];
                memset(buffer, 0xFF, sizeof(buffer));
                sd_card_open(&_sd_ctx, sd_xmodem_conf.filename, true);
                sd_card_write(&_sd_ctx, buffer, sizeof(buffer));
                sd_card_close(&_sd_ctx);
            }
        }
        printf("Transfer aborted\n");
        if (!_inhibit_35mm_messages)
            bs_printf(&_bs_ctx, "Transfer aborted\n");
    }

    if (sd_xmodem_conf.file_open)
    {
        sd_card_close(&_sd_ctx);
        sd_xmodem_conf.file_open = false;
    }
  
    return 0;
}

static bool backup_zc95_firmware(const char* backup_filename)
{
    bs_printf(&_bs_ctx, "Writing current firmware to %s...\n", backup_filename);

    if (!sd_card_open(&_sd_ctx, backup_filename, true))
    {
        bs_printf(&_bs_ctx, "Failed!\n");
        return false;
    }

    printf("Start backup .");
    leds_show_percent(0, COLOUR_BLUE);
    uint8_t* flash_ptr = (uint8_t*)XIP_BASE + PROGRAM_OFFSET;
    uint8_t progress_percent = 0;
    size_t total_byte_counter = 0;
    while (flash_ptr < (uint8_t*)(XIP_BASE + PICO_FLASH_SIZE_BYTES))
    {
        size_t bytes_to_write = (XIP_BASE + PICO_FLASH_SIZE_BYTES) - (size_t)flash_ptr;
        if (bytes_to_write > 1024)
            bytes_to_write = 1024;
        
        total_byte_counter += bytes_to_write;
        if (!sd_card_write(&_sd_ctx, flash_ptr, bytes_to_write))
        {
            bs_printf(&_bs_ctx, "Failed!\n");
            sd_card_close(&_sd_ctx);
            return false;
        }
        

        uint8_t new_progress_percent = ((total_byte_counter * 100) / (PICO_FLASH_SIZE_BYTES - PROGRAM_OFFSET));
        if (new_progress_percent != progress_percent)
        {
            bs_printf(&_bs_ctx, "Written %d k bytes, %d %%\n", total_byte_counter / 1024, progress_percent);
            progress_percent = new_progress_percent;
            leds_show_percent(progress_percent, COLOUR_BLUE);
        }
        
        flash_ptr += bytes_to_write;
    }

    leds_show_percent(100, COLOUR_BLUE);
    bs_printf(&_bs_ctx, "Firmware saved\n");

    return sd_card_close(&_sd_ctx);
}

static bool copy_sd_card_file_to_flash(const char* filename)
{
    uint8_t buffer[256];
    uint8_t first_block_contents[256] = {0};

    bs_printf(&_bs_ctx, "Writing %s to flash...\n", filename);

    if (!sd_card_open(&_sd_ctx, filename, false))
    {
        bs_printf(&_bs_ctx, "Failed!\n");
        return false;
    }

    size_t filesize_bytes = sd_card_filesize(&_sd_ctx);

    leds_show_percent(0, COLOUR_YELLOW);
    clear_firmware_in_flash();

    size_t bytes_read;
    bool result = false;
    size_t total_byte_counter = 0;
    uint8_t dbg_counter=1;
    bool first_block = true;

    do
    {
        bytes_read = 0;
        result = sd_card_read(&_sd_ctx, buffer, sizeof(buffer), &bytes_read);
        total_byte_counter += 256;
        
        if (bytes_read > 0)
        {
            if (first_block)
            {
                first_block = false;
                memcpy(first_block_contents, buffer, sizeof(first_block_contents));
            }
            else
                write_buffer_to_flash(buffer, total_byte_counter);
        }
        
        if (!dbg_counter++)
        {
            uint8_t progress_percent = ((total_byte_counter * 100) / filesize_bytes);
            bs_printf(&_bs_ctx, "Written %d k bytes, %d %%\n", total_byte_counter / 1024, progress_percent);
            leds_show_percent(progress_percent, COLOUR_YELLOW);
        }

    } while (bytes_read == sizeof(buffer) && result);

    // Now write the first block
    if (result)
    {
        write_buffer_to_flash(first_block_contents, 256);
        printf("Write complete, ~ %d bytes\n", total_byte_counter);
        bs_printf(&_bs_ctx, "Firmware updated.\n");
        leds_show_percent(100, COLOUR_YELLOW);
    }
    else
    {
        // read failed
        bs_printf(&_bs_ctx, "ERROR: SD read error - firmware wiped but not updated!\n");
        leds_show_update_error();
    }

    return sd_card_close(&_sd_ctx) && result;
}

enum boot_mode_requested_t serial_input_at_boot()
{
    uint8_t buffer[8] = {0};

    bs_read(&_bs_ctx, buffer, sizeof(buffer));

    for (uint8_t n=0; n < sizeof(buffer); n++)
    {
        if (buffer[n] == 0x1B) // ESC
            return BOOT_ESC;

        if (buffer[n] == ACK)
            return BOOT_ACK;
    }

    return BOOT_NA;
}

void check_eeprom_flag()
{
    uint8_t eeprom_flag = get_eeprom_flag();
    bool ret;

    switch(eeprom_flag)
    {
        // There should be a new f/w in the PENDING file. Backup the current f/w, copy the new f/w to flash, then launch it.
        case EEPROM_BOOTLOADER_SETTING_RESTORE_PENDING:
        {
            bs_printf(&_bs_ctx, "EEPROM bootloader flag set to RESTORE_PENDING\n");
            
            // If something goes wrong _before_ we wipe the current f/w, just boot up as normal next time
            set_eeprom_flag(EEPROM_BOOTLOADER_SETTING_NORMAL);

            if (!backup_zc95_firmware(FILENAME_FW95_PREV))
            {
                bs_printf(&_bs_ctx, "Backup of current zc95 firmware failed, not doing update\n");
                launch_firmware();
            }

            if (!zc624_backup_firmware(&_sd_ctx, FILENAME_FW624_PREV))
            {
                bs_printf(&_bs_ctx, "Backup of current zc624 firmware failed, not doing update\n");
                launch_firmware();
            }

            // Before wiping the f/w in flash, set the flag so that on next boot, we restore the previous f/w.
            // If the new firmware is successfully copied to flash and launched, it'll set this back to NORMAL. 
            set_eeprom_flag(EEPROM_BOOTLOADER_SETTING_RESTORE_PREV);

            if (copy_sd_card_file_to_flash(FILENAME_FW95_PEND) && zc624_send_sd_card_file(&_sd_ctx, FILENAME_FW624_PEND))
                launch_firmware();
            else
            {
                bs_printf(&_bs_ctx, "Update of firmware failed!\n");
                die_error();
            }

            break;
        }

        case EEPROM_BOOTLOADER_SETTING_RESTORE_PREV:
            bs_printf(&_bs_ctx, "EEPROM bootloader flag set to RESTORE_PREVIOUS\n");

            ret =  copy_sd_card_file_to_flash(FILENAME_FW95_PREV);
            ret &= zc624_send_sd_card_file(&_sd_ctx, FILENAME_FW624_PREV);
            if (ret)
                launch_firmware();
            else
            {
                bs_printf(&_bs_ctx, "Restore of previous firmware failed!\n");
                die_error();
            }

            break;

        case EEPROM_BOOTLOADER_SETTING_NORMAL:
        default:
            return;
    }
}

uint8_t show_inital_menu()
{
    char entry = 0;
    do 
    {
        bs_printf(&_bs_ctx, "\n\nZC95 bootloader\n");
        bs_clear_rx_buffer(&_bs_ctx);

        bs_printf(&_bs_ctx, "[ 1 ] Manage ZC95 firmware\n");
        bs_printf(&_bs_ctx, "[ 2 ] Manage ZC624/output firmware\n");

        bs_printf(&_bs_ctx, "\nOption: ");

        while (!bs_read(&_bs_ctx, &entry, 1));
    } while (entry < '1' || entry > '2');

    bs_printf(&_bs_ctx, "\n");

    return entry - '1' + 1; // should be 1 - 2 for '1' (0x61) to '2' (0x62)
}

uint8_t show_zc95_menu()
{
    firmware_info_t fw_info_orig;
    firmware_info_t fw_info_prev;
    firmware_info_t fw_info_pend;
    firmware_info_t fw_info_flash;

    if (_have_sd_card) 
    {
        get_sd_firmware_version(&_sd_ctx, FILENAME_FW95_ORIG, &fw_info_orig, FW_TYPE_95);
        get_sd_firmware_version(&_sd_ctx, FILENAME_FW95_PREV, &fw_info_prev, FW_TYPE_95);
        get_sd_firmware_version(&_sd_ctx, FILENAME_FW95_PEND, &fw_info_pend, FW_TYPE_95);
    }
    get_flash_firmware_version(&fw_info_flash);

    char entry = 0;
    do 
    {
        bs_printf(&_bs_ctx, "\n\nChange ZC95 main firmware\n");
        if (!_have_sd_card) bs_printf(&_bs_ctx, "(no SD card detected)\n");
        
        bs_clear_rx_buffer(&_bs_ctx);

        if (_have_sd_card) 
        {
            bs_printf(&_bs_ctx, "[ 1 ] Restore original f/w (%s)\n", fw_info_orig.firmware_version);
            bs_printf(&_bs_ctx, "[ 2 ] Restore prev f/w     (%s)\n", fw_info_prev.firmware_version);
            bs_printf(&_bs_ctx, "[ 3 ] Restore pending f/w  (%s)\n", fw_info_pend.firmware_version);
            bs_printf(&_bs_ctx, "[ 4 ] Upload f/w to pending slot\n");
        }
        
        bs_printf(&_bs_ctx, "[ 5 ] Upload f/w direct to flash\n");
            
        if (_have_sd_card) bs_printf(&_bs_ctx, "[ 6 ] Backup current f/w to prev slot\n");
        bs_printf(&_bs_ctx, "[ 7 ] Launch f/w           (%s)\n", fw_info_flash.firmware_version);
        bs_printf(&_bs_ctx, "[ 8 ] Back\n");

        bs_printf(&_bs_ctx, "\nOption: ");

        while (!bs_read(&_bs_ctx, &entry, 1));
    } while (entry < '1' || entry > '8');

    bs_printf(&_bs_ctx, "\n");

    return entry - '1' + 1; // should be 1 - 8 for '1' (0x61) to '8' (0x68)
}

uint8_t show_zc624_menu()
{
    firmware_info_t fw_info_orig;
    firmware_info_t fw_info_prev;
    firmware_info_t fw_info_pend;
    firmware_info_t fw_info_flash;

    if (_have_sd_card) 
    {
        get_sd_firmware_version(&_sd_ctx, FILENAME_FW624_ORIG, &fw_info_orig, FW_TYPE_624);
        get_sd_firmware_version(&_sd_ctx, FILENAME_FW624_PREV, &fw_info_prev, FW_TYPE_624);
        get_sd_firmware_version(&_sd_ctx, FILENAME_FW624_PEND, &fw_info_pend, FW_TYPE_624);
    }
    get_flash_firmware_version(&fw_info_flash);

    char zc624_version_string[ZC624_REG_VERSTREND-ZC624_REG_VERSTRSTART + 1] = {0};
    zc624_get_version(NULL, NULL, zc624_version_string, sizeof(zc624_version_string));

    char entry = 0;
    do 
    {
        bs_printf(&_bs_ctx, "\n\nChange ZC624/output main firmware\n");
        if (!_have_sd_card) bs_printf(&_bs_ctx, "(no SD card detected)\n");

        bs_clear_rx_buffer(&_bs_ctx);

        if (_have_sd_card) 
        {
            bs_printf(&_bs_ctx, "[ 1 ] Restore original f/w (%s)\n", fw_info_orig.firmware_version);
            bs_printf(&_bs_ctx, "[ 2 ] Restore prev f/w     (%s)\n", fw_info_prev.firmware_version);
            bs_printf(&_bs_ctx, "[ 3 ] Restore pending f/w  (%s)\n", fw_info_pend.firmware_version);
            bs_printf(&_bs_ctx, "[ 4 ] Upload f/w to pending slot\n");
        }
        bs_printf(&_bs_ctx, "[ 5 ] Upload f/w direct to flash\n");
        
        if (_have_sd_card) bs_printf(&_bs_ctx, "[ 6 ] Backup current f/w to prev slot\n");
        bs_printf(&_bs_ctx, "[ 7 ] Start firmware       (%s)\n", zc624_version_string);
        bs_printf(&_bs_ctx, "[ 8 ] Back\n");

        bs_printf(&_bs_ctx, "\nOption: ");

        while (!bs_read(&_bs_ctx, &entry, 1));
    } while (entry < '1' || entry > '8');

    bs_printf(&_bs_ctx, "\n");

    return entry - '1' + 1; // should be 1 - 8 for '1' (0x61) to '8' (0x68)
}

bool handle_zc95_menu_entry(uint8_t selection)
{
    switch(selection)
    {
        case MENU_OPT_RESTORE_ORIG:
            if (_have_sd_card) copy_sd_card_file_to_flash(FILENAME_FW95_ORIG);
            break;

        case MENU_OPT_RESTORE_PREV:
            if (_have_sd_card) copy_sd_card_file_to_flash(FILENAME_FW95_PREV);
            break;

        case MENU_OPT_RESTORE_PEND:
            if (_have_sd_card) copy_sd_card_file_to_flash(FILENAME_FW95_PEND);
            break;

        case MENU_OPT_UPLOAD_PEND:
            if (_have_sd_card) download_firmware(FW_TARGET_SD_CARD, FILENAME_FW95_PEND);
            break;

        case MENU_OPT_UPLOAD_FLASH:
            download_firmware(FW_TARGET_FLASH, NULL);
            break;

        case MENU_OPT_BACKUP_TO_PREV:
            if (_have_sd_card) backup_zc95_firmware(FILENAME_FW95_PREV);
            break;

        case MENU_OPT_LAUNCH_FW:
            bs_printf(&_bs_ctx, "\nLaunching main firmware... \n");
            if (_have_sd_card) sd_card_deinit(&_sd_ctx);
            launch_firmware();
            break;

        case MENU_OPT_BACK:
            return false;
    }

    return true;
}

bool handle_zc624_menu_entry(uint8_t selection)
{
    switch(selection)
    {
        case MENU_OPT_RESTORE_ORIG:
            if (_have_sd_card) zc624_send_sd_card_file(&_sd_ctx, FILENAME_FW624_ORIG);
            break;

        case MENU_OPT_RESTORE_PREV:
            if (_have_sd_card) zc624_send_sd_card_file(&_sd_ctx, FILENAME_FW624_PREV);
            break;

        case MENU_OPT_RESTORE_PEND:
            if (_have_sd_card) zc624_send_sd_card_file(&_sd_ctx, FILENAME_FW624_PEND);
            break;

        case MENU_OPT_UPLOAD_PEND:
            if (_have_sd_card) download_firmware(FW_TARGET_SD_CARD, FILENAME_FW624_PEND);
            break;

        case MENU_OPT_UPLOAD_FLASH:
            download_firmware(FW_TARGET_624, NULL);
            break;

        case MENU_OPT_BACKUP_TO_PREV:
            if (_have_sd_card) zc624_backup_firmware(&_sd_ctx, FILENAME_FW624_PREV);
            break;

        case MENU_OPT_LAUNCH_FW:    
            {
                uint8_t data_buffer[2];
                data_buffer[0] = ZC624_REG_BOOTLOADER;
                data_buffer[1] = ZC624_REG_BOOTLOADER_STATE_RUN_MAIN_FIRMWARE;
                i2c_write(__func__, ZC624_ADDR, data_buffer, sizeof(data_buffer), false);
            }
            break;

        case MENU_OPT_BACK:
            return false;
    }

    return true;
}

void get_message(uint8_t* out_message, size_t length)
{
    uint8_t entry;
    size_t bytes_read = 0;

    // Wait for STX
    do
    {
        bytes_read = bs_read(&_bs_ctx, &entry, 1);

    } while (bytes_read == 0 || entry != STX);
    

    // Copy everything until next ETX into out_message
    size_t message_length_count = 0;
    do
    {
        bytes_read = bs_read(&_bs_ctx, &entry, 1);
        if (bytes_read == 1 && entry != ETX)
        {
            *(out_message + message_length_count++) = entry;
        }

    } while ((bytes_read == 0 || entry != ETX) && (message_length_count < length - 1));
}

void process_message(uint8_t* message)
{
    printf("Got message: [%s]\n", message);

    if (!strcmp(message, "95:05"))
        handle_zc95_menu_entry(MENU_OPT_UPLOAD_FLASH);

    else if (!strcmp(message, "624:05"))
        handle_zc624_menu_entry(MENU_OPT_UPLOAD_FLASH);

    else if (!strcmp(message, "95:07"))
        handle_zc95_menu_entry(MENU_OPT_LAUNCH_FW);

    else // unknown message (in this version)
        // NAK would seem like a good thing to send, but that's also how 
        // XMODEM transfers are started, so probably not good choice
        bs_printf(&_bs_ctx, "%cERR%c\n", STX, ETX);
}

// On the mk1, audio input (if the optional audio board is installed) and serial input share the same socket.
// For this bootloader to be of any use, we need to make sure it's configured for serial input on power up.
// On MK2's, this'll do nothing.
void mk1_disable_audio_input()
{
    uint8_t data[1];
    data[0] = 0;
    i2c_write(__func__, MK1_CONTROLS_PORT_EXP_ADDR, data, sizeof(data), false);
}

int main()
{
    i2c_start();
    mk1_disable_audio_input();
    sleep_ms(10);
    leds_init();
    sleep_ms(1);
    led_startup(); // set the left and right most LEDs to purple to indicate the bootloader has started

    // Serial on acc port - used for debug output (debug printf's)
    gpio_set_function(PIN_ACC_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_ACC_UART_RX, GPIO_FUNC_UART);
    stdio_uart_init_full(ACC_PORT_UART, PICO_DEFAULT_UART_BAUD_RATE, PIN_ACC_UART_TX, PIN_ACC_UART_RX);
    printf("Startup - ZC95 bootloader version: %s\n", firmware_info.firmware_version);

    firmware_info_t fw_info_flash;
    get_flash_firmware_version(&fw_info_flash);
    printf("          ZC95 main version      : %s\n", fw_info_flash.firmware_version);

    // serial on 3.5mm, for actual uploads / control
    gpio_set_function(PIN_AUX_UART_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_AUX_UART_RX, GPIO_FUNC_UART);
    bs_init(&_bs_ctx, AUX_PORT_UART);

    bs_printf(&_bs_ctx, "\nZC95 bootloader version %s\n", firmware_info.firmware_version);
    bs_printf(&_bs_ctx, "Hold ESC key to enter bootloader menu whilst powering on\n");
    bs_printf(&_bs_ctx, "%c\n", SYN); // Send SYN for the benefit of upload tool
    sleep_ms(100); 
    uint8_t boot_mode = serial_input_at_boot();
    if (!sd_card_init(&_sd_ctx))
    {
        _have_sd_card = false;
        printf("No SD card detected\n");
    }
    else
    {
        _have_sd_card = true;
    }
  
    if (boot_mode == BOOT_NA)
    {
        // Neither ACK or ESC being sent over serial
        bs_printf(&_bs_ctx, "Continuing.\n");

        // If a flag has been set in eeprom to upgrade/restore the f/w, then do that before launching the f/w
        check_eeprom_flag();

        // Check if the zc624 firmware is compatable with the zc95 firmware. If not, and a compatable firmware 
        // can be found on the SD card, load it
        if (_have_sd_card) zc624_validate_and_update_version(&_sd_ctx);

        if (is_firmware_in_flash_valid())
        {
            if (_have_sd_card) sd_card_deinit(&_sd_ctx);
            launch_firmware(); // should never return
            while(1);
        }
        else
        {
            printf("Invalid f/w\n");
            bs_printf(&_bs_ctx, "WARNING: firmware in flash does not appear to be valid. Not starting.\n");
        }
    }
    else if (boot_mode == BOOT_ACK)
    {
        printf("Received ACK on 3.5mm serial\n");
        set_eeprom_flag(EEPROM_BOOTLOADER_SETTING_NORMAL); // If the utility is being used to upload f/w, don't confuse things by trying to auto-update after next reboot
        _inhibit_35mm_messages = true; // supress a lot of status messages sent over the 3.5mm serial that will disrupt the upload tool

        while (1)
        {
            uint8_t message[10] = {0};
            get_message(message, sizeof(message));
            process_message(message);
        }
    }
    else
    {
        // If ESC has been pressed, clear the bootloader eeprom flag to avoid getting stuck if things have gone wrong
        set_eeprom_flag(EEPROM_BOOTLOADER_SETTING_NORMAL); 
    }
    
    sleep_ms(200);
    
    printf("Showing bootloader menu over 3.5mm serial\n");
    while (1)
    {
        int initial_entry = show_inital_menu();
        if (initial_entry == MENU_INITIAL_ZC95)
        {
            while (handle_zc95_menu_entry(show_zc95_menu())) 
                ;
        }
        else // MENU_INITIAL_ZC624
        {
            while (handle_zc624_menu_entry(show_zc624_menu())) 
                ;
        }

    }
}
