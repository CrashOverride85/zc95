#ifndef _ZC624_H_
#define _ZC624_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "zc_types.h"
#include "sdcard/sd_card.h"

bool zc624_read_register(uint8_t reg, uint8_t* out_contents);
bool zc624_get_version(uint8_t* out_major_ver, uint8_t* out_minor_ver, char* out_version, size_t version_size);
bool zc624_xmodem_data_received_send(const uint8_t* data, size_t length, void* user);
bool zc624_send_buffer(const uint8_t* buffer, uint8_t size, uint8_t block);
bool zc624_clear_firmware_on(bool xmodem_xfer_in_progress);
bool zc624_get_fw_block_count(uint16_t* out_block_count);
bool zc624_backup_firmware(struct sdcard_ctx* sd_ctx, const char* backup_filename);
bool zc624_send_sd_card_file(struct sdcard_ctx* sd_ctx, const char* filename);
void zc624_load_firmware_info_from_sd(struct sdcard_ctx* sd_ctx, struct sd_firmware_t fw_versions[3]);
void zc624_validate_and_update_version(struct sdcard_ctx* sd_ctx);
const char* zc624_get_firmware_filename(struct sdcard_ctx* sd_ctx, uint8_t major_version, uint8_t minor_version, char* version_string);

#endif
