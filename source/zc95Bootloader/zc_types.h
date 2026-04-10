#ifndef _ZC_TYPES_H_
#define _ZC_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "git_version.h"

enum fw_target_e
{
    FW_TARGET_SD_CARD,
    FW_TARGET_FLASH,
    FW_TARGET_624
};

enum fw_type_e
{
    FW_TYPE_95,
    FW_TYPE_624
};

enum boot_mode_requested_t
{
    BOOT_NA ,  // No (relevant) input received
    BOOT_ESC,  // Escape pressed - show menu
    BOOT_ACK   // ACK byte received - request from upload utility to enter bootloader mode
};

typedef struct
{
    const char* filename;
    bool file_open;
} bootloader_xmodem_t;

typedef struct
{
    uint8_t block;
    int total_byte_counter;
} bootloader_xmodem_624_t;

struct sd_firmware_t
{
    firmware_info_t info;
    const char* filename;
    bool is_valid;
};

extern const char* FILENAME_FW95_ORIG;
extern const char* FILENAME_FW95_PREV;
extern const char* FILENAME_FW95_PEND;
extern const char* FILENAME_FW624_ORIG;
extern const char* FILENAME_FW624_PREV;
extern const char* FILENAME_FW624_PEND;


#endif
