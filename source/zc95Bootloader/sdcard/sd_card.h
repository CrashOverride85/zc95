#ifndef _SD_CARD_H
#define _SD_CARD_H

#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
#include "hw_config.h"


struct sdcard_ctx
{
    sd_card_t *pSD;
    FIL file;
    size_t filesize;
};


bool sd_card_init(struct sdcard_ctx* ctx);
bool sd_card_open(struct sdcard_ctx* ctx, const char* filename, bool write);
size_t sd_card_filesize(struct sdcard_ctx* ctx);
bool sd_card_write(struct sdcard_ctx* ctx, const uint8_t* data, size_t length);
bool sd_card_read(struct sdcard_ctx* ctx, uint8_t* buffer, size_t bytes_to_read, size_t* bytes_read);
bool sd_card_close(struct sdcard_ctx* ctx);
void sd_card_deinit(struct sdcard_ctx* ctx);

#endif
