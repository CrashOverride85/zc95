#include "CSdCard.h"

CSdCard::CSdCard()
{

}

CSdCard::~CSdCard()
{

}

void CSdCard::mount()
{
   sd_card_t* pSD = sd_get_by_num(0);

    FRESULT result = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != result)
    {
        printf("failure: f_mount error: %s (%d)\n", FRESULT_str(result), result);
        return;
    }
    
    printf("SD card mounted\n");

    // Create Lua directory
    result = f_mkdir("Lua");
    if (FR_OK != result)
    {
        printf("failure: f_mkdir error: %s (%d)\n", FRESULT_str(result), result);
        return;
    }


}

void CSdCard::dir_list()
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int nfile, ndir;


    res = f_opendir(&dir, "");                   /* Open the directory */
    if (res == FR_OK) {
        nfile = ndir = 0;
        for (;;) {
            res = f_readdir(&dir, &fno);           /* Read a directory item */
            if (fno.fname[0] == 0) break;          /* Error or end of dir */
            if (fno.fattrib & AM_DIR) {            /* It is a directory */
                printf("   <DIR>   %s\n", fno.fname);
                ndir++;
            } else {                               /* It is a file */
                printf("%10u %s\n", fno.fsize, fno.fname);
                nfile++;
            }
        }
        f_closedir(&dir);
        printf("%d dirs, %d files.\n", ndir, nfile);
    } else {
        printf("Failed to open \"%s\". (%u)\n", "", res);
    }
    return;
}

bool CSdCard::write_file(std::string filename, const char* contents, size_t len)
{
    FRESULT res;
    FIL file;
    uint bytes_written=0;
    std::string filepath = "0:Lua/" + filename + ".lua";

    printf("CSdCard::write_file: writing %d bytes to %s\n", len, filepath.c_str());

    res = f_open(&file, filepath.c_str(), FA_CREATE_ALWAYS | FA_WRITE);
    if (res)
    {
        printf("failure: f_open error: %s (%d)\n", FRESULT_str(res), res);
        return false;
    } 

    res = f_write(&file, contents, len, &bytes_written);
    if (bytes_written < len || res) 
    {
        printf("failure: f_write error: %s (%d). Len=%u, written=%u\n", FRESULT_str(res), res, len, bytes_written);
    }  

    res = f_close(&file);
    if (res)
    {
        printf("failure: f_open error: %s (%d)\n", FRESULT_str(res), res);
        return false;
    } 

    return true;
}
