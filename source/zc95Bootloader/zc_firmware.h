#ifndef _ZC_FIRMWARE_H_
#define _ZC_FIRMWARE_H_

#include "zc_types.h"
#include "sdcard/sd_card.h"

bool get_sd_firmware_version(struct sdcard_ctx* sd_ctx, const char* filename, firmware_info_t* fw_info, enum fw_type_e fw_type);
void get_flash_firmware_version(firmware_info_t* fw_info);

#endif
