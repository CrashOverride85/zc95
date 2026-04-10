/*
 * ZC95
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
 * 
 * Functions related to communicating with and updating the zc624 (output)
 * pico over I2C. 
 * 
 */

#include "zc624.h"
#include "i2c.h"
#include "zc_types.h"
#include "zc_debug.h"
#include "leds.h"
#include "zc_firmware.h"
#include "../common/zc95_config.h"
#include "../common/i2cEnums.h"
#include "../common/FirmwareMagicNumber.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
 
bool zc624_read_register(uint8_t reg, uint8_t* out_contents)
{
    uint8_t data_buffer[1];
    data_buffer[0] = reg;

    // Write mem address
    int bytes_written = i2c_write(__func__, ZC624_ADDR, data_buffer, sizeof(data_buffer), true);
    if (bytes_written != sizeof(data_buffer))
        return false;

    // Read contents
    int bytes_read = i2c_read(__func__, ZC624_ADDR, data_buffer, sizeof(data_buffer), false);
    if (bytes_read != sizeof(data_buffer))
        return false;

    *out_contents = data_buffer[0];
    return true;
}


bool zc624_get_version(uint8_t* out_major_ver, uint8_t* out_minor_ver, char* out_version, size_t version_size)
{
    bool success = true;
    bool retval;
    uint8_t retry_count = 0;

    // It can take the zc624 a little while to be ready, so give it some time if needed.
    printf("Waiting for zc624 to be ready\n");
    do
    {
        uint8_t val;
        retval = zc624_read_register(ZC624_REG_BOOTLOADER, &val);
        retval &= (val == ZC624_REG_BOOTLOADER_STATE_IN_BOOTLOADER); 

        if (val == ZC624_REG_BOOTLOADER_STATE_RUN_MAIN_FIRMWARE)
        {
            // This should not happen.
            printf("ZC624 has already exited bootloader mode!\n");
            return false;
        }

        if (!retval)
            sleep_ms(25);
    } while (!retval && retry_count++ < 10);

    do
    {
        if (out_version != NULL)
        {
            // get version string
            uint8_t ver_str_len = (ZC624_REG_VERSTREND - ZC624_REG_VERSTRSTART);
            if (ver_str_len > version_size)
                ver_str_len = version_size;

            retval = get_i2c_register_range(ZC624_REG_VERSTRSTART, (uint8_t*)out_version, ver_str_len);
            if (!retval)
            {
                success = false;
                break;
            }
        }

        if (out_major_ver != NULL)
        {
            retval = zc624_read_register(ZC624_REG_VERSIONMAJOR, out_major_ver);
            if (!retval)
            {
                success = false;
                break;
            }
        }

        if (out_minor_ver != NULL)
        {
            retval = zc624_read_register(ZC624_REG_VERSIONMINOR, out_minor_ver);
            if (!retval)
            {
                success = false;
                break;
            }
        }
    } while(0);

    if (success)
        return true;

    strncpy(out_version, "<error>", version_size-1);
    *out_major_ver = 0xFF;
    *out_minor_ver = 0xFF;
    
    return false;
}

bool zc624_xmodem_data_received_send(const uint8_t* data, size_t length, void* user)
{
    bootloader_xmodem_624_t* conf = user;

    // start by clearing flash
    if (data != NULL && conf->total_byte_counter == 0)
    {
        if (!zc624_clear_firmware_on(true))
            return false;
    }

    // End of transmission
    if (data == NULL && length == 0)
    {
        // send end of transmission marker
        uint8_t buffer[128] = {0};
        buffer[0] = ZC624_REG_DATABLOCKWRITE;
        buffer[1] = 0x00;
        sleep_ms(50);
        i2c_write(__func__, ZC624_ADDR, buffer, 2, false);

        printf("Transfer complete, ~ %d bytes\n", conf->total_byte_counter);
        if (!_inhibit_35mm_messages) bs_printf(&_bs_ctx, "Transferred %d k bytes, %d %%\n", conf->total_byte_counter / 1024, 100);
        if (!_inhibit_35mm_messages) bs_printf(&_bs_ctx, "Firmware updated.\n");
        return true;
    }

    if (length != 128)
    {
        printf("zc624_xmodem_data_received_send: unexpected data length: %d\n");
        return false;
    }

    if (!zc624_send_buffer(data, length, conf->block++))
    {
        printf("zc624_send_buffer failed, total_byte_counter=%d\n", conf->total_byte_counter);
        return false;
    }
    conf->total_byte_counter += length;

    return true;
}
 
bool zc624_send_buffer(const uint8_t* buffer, uint8_t size, uint8_t block)
{
    uint8_t data_buffer[1 + 1 + 128 + 1]; // i2c command + block number + payload + checksum

    if (size != 128)
    {
        printf("zc624_send_buffer: expected size=128, got %d\n", size);
        return false;
    }

    data_buffer[0] = ZC624_REG_DATABLOCKWRITE;
    data_buffer[1] = block;
    memcpy(data_buffer+2, buffer, size);

    uint8_t checksum=0;
    for (size_t n=0; n < 128; n++)
        checksum += buffer[n];

    data_buffer[sizeof(data_buffer)-1] = checksum;

    return (i2c_write(__func__, ZC624_ADDR, data_buffer, sizeof(data_buffer), false) == sizeof(data_buffer));
}

bool zc624_clear_firmware_on(bool xmodem_xfer_in_progress)
{
    uint8_t reg_val = 0;

    // Check zc624 is in a state where the firmware can be erased
    bool ret = zc624_read_register(ZC624_REG_ERASEFIRMWARE, &reg_val);
    if (!ret || reg_val != ZC624_REG_ERASEFW_ERASABLE)
    {
        if (xmodem_xfer_in_progress)
            printf("zc624 not in valid state to erase\n");
        else
            bs_printf(&_bs_ctx, "zc624 not in valid state to erase\n");
        return false;
    }

    if (!xmodem_xfer_in_progress) bs_printf(&_bs_ctx, "Wiping firmware on zc624...");

    // Send command to start the erase
    uint8_t data_buffer[2]; // i2c command + instruction
    data_buffer[0] = ZC624_REG_ERASEFIRMWARE;
    data_buffer[1] = ZC624_REG_ERASEFW_ERASE;
    int bytes_written = i2c_write(__func__, ZC624_ADDR, data_buffer, sizeof(data_buffer), false);
    if (bytes_written != sizeof(data_buffer))
    {
        bs_printf(&_bs_ctx, "Failed to erase firmware on zc624! (ret=%d)\n", bytes_written);
        return false;
    }

    // Wait for erase to complete
    printf("Expect i2c errors whilst polling for flash erase completion\n");
    uint8_t timeout = 150;
    reg_val = 0;
    do
    {
        sleep_ms(100);
        zc624_read_register(ZC624_REG_ERASEFIRMWARE, &reg_val);
        timeout++;
        
    } while (reg_val != ZC624_REG_ERASEFW_ERASABLE && timeout);

    if (reg_val == ZC624_REG_ERASEFW_ERASABLE)
    {
        if (!xmodem_xfer_in_progress) bs_printf(&_bs_ctx, "done\n");
        return true;
    }
    else
    {
        if (xmodem_xfer_in_progress)
            printf("timeout!\n");
        else
            bs_printf(&_bs_ctx, "timeout!\n");

        return false;
    }
}

bool zc624_get_fw_block_count(uint16_t* out_block_count)
{
    uint8_t data_buffer[2];
    data_buffer[0] = ZC624_REG_FW_BLOCK_COUNT;

    // Write mem address
    int bytes_written = i2c_write(__func__, ZC624_ADDR, data_buffer, 1, true);
    if (bytes_written != 1)
        return false;

    // Read contents
    int bytes_read = i2c_read(__func__, ZC624_ADDR, data_buffer, sizeof(data_buffer), false);
    if (bytes_read != sizeof(data_buffer))
        return false;

    *out_block_count = data_buffer[0];
    *out_block_count |= data_buffer[1] << 8;
    return true;
}

bool zc624_backup_firmware(struct sdcard_ctx* sd_ctx, const char* backup_filename)
{
    bs_printf(&_bs_ctx, "Writing zc624 current firmware to %s...\n", backup_filename);

    // Get size of firmware in terms of block count (1 block = 128 bytes)
    uint16_t block_count;
    if (!zc624_get_fw_block_count(&block_count))
    {
        bs_printf(&_bs_ctx, "Failed to determine firmware size\n");
        return false;
    }

    if (!sd_card_open(sd_ctx, backup_filename, true))
    {
        bs_printf(&_bs_ctx, "Failed!\n");
        return false;
    }
    
    size_t total_byte_counter = 0;
    for (uint16_t blk = 0; blk < block_count; blk++)
    {
        uint8_t request[3];
        request[0] = ZC624_REG_DATABLOCKREAD;
        request[1] = blk & 0xFF;
        request[2] = (blk >> 8) & 0xFF;
        i2c_write(__func__, ZC624_ADDR, request, sizeof(request), true);

        uint8_t receive_buffer[128];

        i2c_read(__func__, ZC624_ADDR, receive_buffer, sizeof(receive_buffer), false);
        total_byte_counter += sizeof(receive_buffer);

        if (!sd_card_write(sd_ctx, receive_buffer, sizeof(receive_buffer)))
        {
            bs_printf(&_bs_ctx, "Failed!\n");
            sd_card_close(sd_ctx);
            return false;
        }

        if (!(blk % 10))
        {
            uint8_t progress_percent = ((total_byte_counter * 100) / (block_count * 128));
            bs_printf(&_bs_ctx, "Received %d k bytes, %d %%\n", total_byte_counter / 1024, progress_percent);
            leds_show_percent(progress_percent, COLOUR_BLUE);
        }
    }

    uint8_t progress_percent = ((total_byte_counter * 100) / (block_count * 128));
    bs_printf(&_bs_ctx, "Received %d k bytes, %d %%\n", total_byte_counter / 1024, progress_percent);
    leds_show_percent(100, COLOUR_BLUE);
    bs_printf(&_bs_ctx, "Firmware saved\n");

    return sd_card_close(sd_ctx);
}

bool zc624_send_sd_card_file(struct sdcard_ctx* sd_ctx, const char* filename)
{
    uint8_t buffer[128];

    leds_show_percent(0, COLOUR_YELLOW);
    
    if (!zc624_clear_firmware_on(false))
        return false;

    bs_printf(&_bs_ctx, "Sending %s to zc624...\n", filename);

    if (!sd_card_open(sd_ctx, filename, false))
    {
        bs_printf(&_bs_ctx, "Failed to open file!\n");
        return false;
    }

    size_t filesize_bytes = sd_card_filesize(sd_ctx);

    size_t bytes_read;
    bool result = false;
    uint8_t dbg_counter=1;
    size_t total_byte_counter = 0;
    uint8_t block = 1;

    do
    {
        bytes_read = 0;
        result = sd_card_read(sd_ctx, buffer, sizeof(buffer), &bytes_read);
        total_byte_counter += bytes_read;
        
        if (bytes_read > 0)
        {
            if (!zc624_send_buffer(buffer, sizeof(buffer), block++))
            {
                bs_printf(&_bs_ctx, "Transfer to ZC624 failed!\n");
                printf("zc624_send_buffer failed, total_byte_counter=%d\n", total_byte_counter);
                sd_card_close(sd_ctx);
                leds_show_update_error();
                return false;
            }
        }
        
        if (!(dbg_counter++ % 10))
        {
            uint8_t progress_percent = ((total_byte_counter * 100) / filesize_bytes);
            bs_printf(&_bs_ctx, "Sent %d k bytes, %d %%\n", total_byte_counter / 1024, progress_percent);
            leds_show_percent(progress_percent, COLOUR_YELLOW);
        }

    } while (bytes_read == sizeof(buffer) && result);

    if (result)
    {
        // send end of transmission marker
        buffer[0] = ZC624_REG_DATABLOCKWRITE;
        buffer[1] = 0x00;
        sleep_ms(50);
        i2c_write(__func__, ZC624_ADDR, buffer, 2, false);

        printf("Write complete, ~ %d bytes\n", total_byte_counter);
        bs_printf(&_bs_ctx, "Sent %d k bytes, %d %%\n", total_byte_counter / 1024, 100);
        bs_printf(&_bs_ctx, "Firmware updated.\n");
        leds_show_percent(100, COLOUR_YELLOW);
    }
    else
    {
        // read failed
        bs_printf(&_bs_ctx, "ERROR: SD read error - firmware likely wiped on zc624 but not updated!\n");
        leds_show_update_error();
    }

    return sd_card_close(sd_ctx) && result;
}

void zc624_load_firmware_info_from_sd(struct sdcard_ctx* sd_ctx, struct sd_firmware_t fw_versions[3])
{
    get_sd_firmware_version(sd_ctx, FILENAME_FW624_PEND, &fw_versions[0].info, FW_TYPE_624);
    get_sd_firmware_version(sd_ctx, FILENAME_FW624_PREV, &fw_versions[1].info, FW_TYPE_624);
    get_sd_firmware_version(sd_ctx, FILENAME_FW624_ORIG, &fw_versions[2].info, FW_TYPE_624);

    fw_versions[0].filename = FILENAME_FW624_PEND;
    fw_versions[1].filename = FILENAME_FW624_PREV;
    fw_versions[2].filename = FILENAME_FW624_ORIG;

    for (int x = 0; x < 3; x++)
    {
        fw_versions[x].is_valid = (fw_versions->info.magic == FIRMWARE_VERSION_624_MAGIC);
    }
}

// Search through the 3 firmware files on SD card. and look for zc624 firmware that is compatible 
// with the zc95 version requirement data passed in.
const char* zc624_get_firmware_filename(struct sdcard_ctx* sd_ctx, uint8_t major_version, uint8_t minor_version, char* version_string)
{
    const int firmware_count = 3;
    struct sd_firmware_t firmware_files[firmware_count];

    zc624_load_firmware_info_from_sd(sd_ctx, firmware_files);

    // Look for an exact match by firmware version string
    for (int n=0; n < firmware_count; n++)
    {
        if (!firmware_files[n].is_valid)
            continue;

        if (!strcmp(firmware_files[n].info.firmware_version, version_string))
        {
            // match found
            printf("Found exact match for ZC624 firmware in [%s]: [%s]\n", firmware_files[n].filename, firmware_files[n].info.firmware_version);
            return firmware_files[n].filename;
        }
    }

    // Look for a exact match of major/minor version
    for (int n=0; n < firmware_count; n++)
    {
        if (!firmware_files[n].is_valid)
            continue;

        if (firmware_files[n].info.fw624_major == major_version && firmware_files[n].info.fw624_minor == minor_version)
        {
            printf("Found ZC624 firmware with matching major/minor versions in [%s]. F/W version: [%s]\n", 
                firmware_files[n].filename, firmware_files[n].info.firmware_version);
            return firmware_files[n].filename;
        }
    }

    // Look for a matching major version and minor the same or higher
    for (int n=0; n < firmware_count; n++)
    {
        if (!firmware_files[n].is_valid)
            continue;

        if (firmware_files[n].info.fw624_major == major_version && firmware_files[n].info.fw624_minor >= minor_version)
        {
            printf("Found ZC624 firmware with matching major, >= minor version in [%s]. F/W version: [%s]\n", 
                firmware_files[n].filename, firmware_files[n].info.firmware_version);
            return firmware_files[n].filename;
        }
    }

    printf("No suitable ZC624 firmware found. Continuing.\n");
    return NULL;
}

// Get the version of the zc624 firmware, and compare it to the version expected by the currently loaded zc95 
// firmware. If it doesn't match, see if one of the stored firmwares on the SD card is a match - and load it 
// if so. If a matching version can't be found, just carry on. The main zc95 firmware will start and show a
// version mismatch error on screen.
void zc624_validate_and_update_version(struct sdcard_ctx* sd_ctx)
{
    uint8_t zc624_major_version;
    uint8_t zc624_minor_version;
    char zc624_version_string[ZC624_REG_VERSTREND-ZC624_REG_VERSTRSTART + 1] = {0};

    if (!zc624_get_version(&zc624_major_version, &zc624_minor_version, zc624_version_string, sizeof(zc624_version_string)))
    {
        printf("Failed to determine ZC624 version. Continuing.\n");
        return;
    }
    else
    {
        printf("ZC624 version: major=%d, minor=%d  : %s\n", zc624_major_version, zc624_minor_version, zc624_version_string);
    }

    // Get the details of the main ZC95 firmware
    firmware_info_t fw_info_flash;
    get_flash_firmware_version(&fw_info_flash);
    if (fw_info_flash.magic != FIRMWARE_VERSION_95_MAGIC)
    {
        // Main firmware doesn't appear to be valid. Don't attempt any zc624 f/w change based on that
        return;
    }

    if (zc624_major_version == fw_info_flash.fw624_major && zc624_minor_version >= fw_info_flash.fw624_minor)
    {
        // This is good! the firmware version loaded on the zc624 seems to be compatable, so nothing to do
        printf("ZC624 firmware is compatable\n");
        return;
    }

    printf("ZC624 firmware does not appear to be compatable with main ZC95 firmware. Looking for suitable firmware to load.\n");

    const char* matching_firmware_file = zc624_get_firmware_filename(sd_ctx, fw_info_flash.fw624_major, fw_info_flash.fw624_minor, fw_info_flash.firmware_version);
    if (matching_firmware_file == NULL)
    {
        printf("No compatable ZC624 firmware found to load. Continuing.\n");
        return;
    }

    if (!zc624_send_sd_card_file(sd_ctx, matching_firmware_file))
    {
        printf("Update of ZC624 failed! continuing.\n");
    }
}
