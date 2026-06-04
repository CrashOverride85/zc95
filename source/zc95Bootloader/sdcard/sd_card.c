#include "sd_card.h"
#include <stdio.h>
#include <string.h>

bool sd_card_init(struct sdcard_ctx* ctx)
{
    ctx->pSD = sd_get_by_num(0);
    ctx->filesize = 0;

    FRESULT result = f_mount(&ctx->pSD->fatfs, ctx->pSD->pcName, 1);
    if (FR_OK != result)
    {
        printf("failure: f_mount error: %s (%d)\n", FRESULT_str(result), result);
        return false;
    }

    return true;
}

bool sd_card_open(struct sdcard_ctx* ctx, const char* filename, bool write)
{
    FRESULT result;

    if (write)
        result = f_open(&ctx->file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    else
        result = f_open(&ctx->file, filename, FA_OPEN_EXISTING | FA_READ);

    if (FR_OK != result && FR_EXIST != result)
    {
        printf("failure: f_open(%s) error: %s (%d)\n", filename, FRESULT_str(result), result);
        return false;
    }

    printf("sd card file %s opened for %s\n", filename, write ? "writing" : "reading");

    return true;
}

size_t sd_card_filesize(struct sdcard_ctx* ctx)
{
    return (f_size(&ctx->file));
}

bool sd_card_write(struct sdcard_ctx* ctx, const uint8_t* data, size_t length)
{
    FRESULT result;

    UINT written = 0;
    result = f_write(&ctx->file, data, length, &written);
    if (result != FR_OK)
    {
        printf("failed to write\n");
        return false;
    }
    return true;
}

bool sd_card_read(struct sdcard_ctx* ctx, uint8_t* buffer, size_t bytes_to_read, size_t* bytes_read)
{
    FRESULT result;
    memset(buffer, 0, bytes_to_read);
    result = f_read(&ctx->file, buffer, bytes_to_read, bytes_read);
    if (result != FR_OK)
    {
        printf("failed to read\n");
        return false;
    }
    return true;
}

bool sd_card_close(struct sdcard_ctx* ctx)
{
    FRESULT result;

    result = f_close(&ctx->file);
    if (FR_OK != result) 
    {
        printf("backup_firmware failure: f_close error: %s (%d)\n", FRESULT_str(result), result);
        return false; // assume backup didn't work / is corrupt
    }

    return true;
}

void sd_card_deinit(struct sdcard_ctx* ctx)
{
    f_unmount(ctx->pSD->pcName);
    printf("sd card closed\n");
}
