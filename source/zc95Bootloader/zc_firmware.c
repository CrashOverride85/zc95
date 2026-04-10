#include "zc_firmware.h"
#include "zc_types.h"
#include "sdcard/sd_card.h"
#include "../common/FirmwareMagicNumber.h"
#include "../common/zc95_config.h"
#include <string.h>

bool get_sd_firmware_version(struct sdcard_ctx* sd_ctx, const char* filename, firmware_info_t* fw_info, enum fw_type_e fw_type)
{
    strcpy(fw_info->firmware_version, "<sd error>"); // will be overwritten later if things go well
    fw_info->magic = 0;

    if (!sd_card_open(sd_ctx, filename, false))
        return false;

    uint8_t buffer[128] = {0};
    size_t bytes_read = 0;
    bool result = sd_card_read(sd_ctx, buffer, sizeof(buffer), &bytes_read);
    if (result)
    {
        memcpy(fw_info, buffer, sizeof(firmware_info_t));
        // Check the magic number is in the firmware info block. If it doesn't match, the 
        // data we've read likely isn't a zc95 firmware image (or it's corrupt)
        
        uint32_t expected_magic_val;
        if (fw_type == FW_TYPE_95)
            expected_magic_val = FIRMWARE_VERSION_95_MAGIC;
        else // zc624
            expected_magic_val = FIRMWARE_VERSION_624_MAGIC;
        
        if (fw_info->magic != expected_magic_val)
        {
            strcpy(fw_info->firmware_version, "<invalid>");
        }
    }

    sd_card_close(sd_ctx);

    return result;
}

void get_flash_firmware_version(firmware_info_t* fw_info)
{
    memcpy(fw_info, (void*)(XIP_BASE + PROGRAM_OFFSET), sizeof(firmware_info_t));
    if (fw_info->magic != FIRMWARE_VERSION_95_MAGIC)  // "ZC95"
    {
        strcpy(fw_info->firmware_version, "<invalid>");
    }
}
