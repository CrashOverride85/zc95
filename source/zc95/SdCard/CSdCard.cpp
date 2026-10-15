#include "CSdCard.h"
#include <string.h>
#include "../LuaScripts/LuaScripts.h"
#include "../../common/zc95_config.h"

CSdCard::CSdCard(CAnalogueCapture* analogue_capture)
{
    _lua_storage = new CLuaStorage();
    _analogue_capture = analogue_capture;

    _file_buffer = (uint8_t*)calloc(LUA_UPLOAD_BUFFER_SIZE, sizeof(uint8_t)); // this is a bit big to allocate on the stack
    if (_file_buffer == NULL)
    {
        panic("CSdCard::CSdCard(): ERROR - calloc returned NULL. Out of memory.");
    }

    _sd_card_available = mount(true);
    if (_sd_card_available)
        printf("CSdCard::CSdCard: SD card detected\n");
    else
        printf("CSdCard::CSdCard: SD card NOT detected\n");

    unmount();
}

CSdCard::~CSdCard()
{
    printf("CSdCard::~CSdCard()\n");

    if (_lua_storage != NULL)
    {
        delete _lua_storage;
        _lua_storage = NULL;
    }

    if (_file_buffer)
    {
        free(_file_buffer);
        _file_buffer = NULL;
    }
}

bool CSdCard::mount(bool force)
{
    if (!_sd_card_available && !force)
        return false;

    _set_analogue_capture_running_on_unmount = _analogue_capture->is_running();
    if (_set_analogue_capture_running_on_unmount)
    {
        _analogue_capture->stop();
    }
    gpio_put(MIPI_DISPLAY_PIN_CS, 1); // stops interference showing on LCD

    sd_card_t* pSD = sd_get_by_num(0);

    FRESULT result = f_mount(&pSD->fatfs, pSD->pcName, 1);
    if (FR_OK != result)
    {
        printf("failure: f_mount error: %s (%d)\n", FRESULT_str(result), result);
        if (_set_analogue_capture_running_on_unmount)
        {
            _analogue_capture->start();
            _set_analogue_capture_running_on_unmount = false;
        }
        return false;
    }
    
    printf("SD card mounted\n");
    _mounted = true;

    // Create Lua directory
    result = f_mkdir("Lua");
    if (FR_OK != result)
    {
        // printf("failure: f_mkdir error: %s (%d)\n", FRESULT_str(result), result);
    }

    return true;
}

void CSdCard::unmount()
{
    sd_card_t* pSD = sd_get_by_num(0);
    FRESULT result = f_unmount(pSD->pcName);
    if (FR_OK != result)
    {
        printf("failure: f_unmount error: %s (%d)\n", FRESULT_str(result), result);
        return;
    }
    sd_deinit_driver();
    
    printf("SD card unmounted\n");
    _mounted = false;

    if (_set_analogue_capture_running_on_unmount)
    {
        _analogue_capture->start();
        _set_analogue_capture_running_on_unmount = false;
    }
}

std::vector<std::string> CSdCard::get_lua_scripts()
{
    std::vector<std::string> result;
    if (!_sd_card_available)
        return result;

    FRESULT res;
    DIR dir;
    FILINFO fno;

    bool was_mounted = _mounted;

    printf("Search for Lua scripts..\n");

    if (!_mounted)
        mount();

    res = f_opendir(&dir, "Lua");                   /* Open the directory */
    if (res == FR_OK) 
    {
        for (;;) 
        {
            res = f_readdir(&dir, &fno);           /* Read a directory item */
            if (fno.fname[0] == 0) 
                break;                             /* Error or end of dir */
            if (fno.fattrib & AM_DIR) 
            {            
                /* It is a directory */
            } 
            
            else 
            {                               
                /* It is a file */
                printf("\t %s\t", fno.fname);
                if (remove_lua_extension(fno.fname))
                {
                    printf("{added}\n");
                    result.push_back("U:" + std::string(fno.fname));
                }
                else
                {
                    printf("\n");
                }
            }
        }
        f_closedir(&dir);
    } 
    
    else 
    {
        printf("failure: f_opendir error: %s (%d)\n", FRESULT_str(res), res);
    }

    if (!was_mounted)
        unmount();

    return result;
}

bool CSdCard::write_file(std::string filename, const char* contents, size_t len, std::string& error)
{
    if (!_sd_card_available)
    {
        error = "No SD card found";
        return false;
    }

    FRESULT res;
    FIL file;
    uint bytes_written=0;
    std::string filepath = "0:Lua/" + filename.substr(0, LUA_ON_SD_SCRIPT_MAX_FILE_LEN) + ".lua";

    printf("CSdCard::write_file: writing %d bytes to %s\n", len, filepath.c_str());

    bool retval = false;
    bool was_mounted = _mounted;
    if (!_mounted)
        mount();

    res = f_open(&file, filepath.c_str(), FA_CREATE_ALWAYS | FA_WRITE);
    if (res)
    {
        error = "SD file open error: " + std::string(FRESULT_str(res));
        printf("failure: f_open error: %s (%d)\n", FRESULT_str(res), res);
    }
    
    else
    {
        res = f_write(&file, contents, len, &bytes_written);
        if (bytes_written < len || res) 
        {
            error = "SD write error: " + std::string(FRESULT_str(res));
            printf("failure: f_write error: %s (%d). Len=%u, written=%u\n", FRESULT_str(res), res, len, bytes_written);
        }  

        res = f_close(&file);
        if (res)
        {
            error = "SD file close error: " + std::string(FRESULT_str(res));
            printf("failure: f_close error: %s (%d)\n", FRESULT_str(res), res);
        }
        else
        {
            error = "";
            retval = true;
        }
    }

    if (!was_mounted)
        unmount();

    return retval;
}

bool CSdCard::copy_file_to_flash(std::string script_title)
{
    if (!_sd_card_available)
        return false;

    if (script_title == _script_in_flash)
    {
        printf("CSdCard::copy_file_to_flash: script [%s] already loaded into flash, nothing to do\n", script_title.c_str());
        return true; // nothing's gone wrong, so consider this a success
    }

    uint8_t staging_slot = sd_staging_slot();
    FIL fsrc;
    UINT bytes_read;

    bool was_mounted = _mounted;
    if (!_mounted)
        mount();

    _lua_storage->delete_script_at_index(staging_slot);
    
    std::string filepath = "0:Lua/" + script_title.substr(2) + ".lua"; // substr to strip off "U:" prefix

    printf("CSdCard::copy_file_to_flash: copying file [%s] to lua flash slot %d\n", filepath.c_str(), staging_slot);

    FRESULT result = f_open(&fsrc, filepath.c_str(), FA_READ);
    if (result)
    {
        printf("failure: f_open error: %s (%d)\n", FRESULT_str(result), result);
        if (!was_mounted)
            unmount();
        return false;
    } 

    // Copy file to flash
    uint8_t section = 0;
    bool store_result = true;
    for (;;) 
    {
        memset(_file_buffer, 0, LUA_UPLOAD_BUFFER_SIZE);
        result = f_read(&fsrc, _file_buffer, LUA_UPLOAD_BUFFER_SIZE, &bytes_read); /* Read a chunk of data from the source file */
        if (bytes_read == 0) break; // error or eof

        store_result = _lua_storage->store_script_section(staging_slot, section, (const char*)_file_buffer, LUA_UPLOAD_BUFFER_SIZE);

        if (!store_result)
        {
            printf("CSdCard::copy_file_to_flash: failed to write section %d to flash\n", section);
            break;
        }
        
        section++;
    }

    bool retval = true;
    if (result)
    {
        printf("failure: f_read error: %s (%d)\n", FRESULT_str(result), result);
        retval = false;
    }

    result = f_close(&fsrc);
    if (result)
    {
        printf("failure: f_close error: %s (%d)\n", FRESULT_str(result), result);
        retval = false;
    } 

    if (!was_mounted)
        unmount();

    if (retval)
        _script_in_flash = script_title;

    printf("CSdCard::copy_file_to_flash: done copying file from SD to flash\n");
    return retval;
}

bool CSdCard::delete_scripts_at_index(std::set<uint> indexes)
{
    if (!_sd_card_available)
        return false;

    bool retval = false;
    bool was_mounted = _mounted;
    if (!_mounted)
        mount();

    std::vector<std::string> scripts = get_lua_scripts();

    // Generate list of scripts to delete
    std::vector<std::string> filenames_to_delete;
    for (uint index : indexes)
    {
        std::string filename = get_filepath_from_index(scripts, index);
        if (filename != "")
        {
            filenames_to_delete.push_back(filename);
        }
    }

    // Delete scripts
    retval = true;
    for (std::string& filename : filenames_to_delete)
    {
        printf("CSdCard::delete_scripts_at_index: deleteing [%s]\n", filename.c_str());
        FRESULT result = f_unlink(filename.c_str());
        if (result)
        {
            printf("failure: f_unlink error: %s (%d)\n", FRESULT_str(result), result);
            retval = false;
        }
    }

    if (!was_mounted)
        unmount();

    return retval;
}

uint8_t CSdCard::sd_staging_slot()
{
    int sd_staging_slot = get_sd_staging_slot();
    if (sd_staging_slot < 0 || sd_staging_slot >= 255)
    {
        panic("CSdCard::sd_staging_slot(): Failed to determine SD card staging slot in flash");
    }
    return (uint8_t)sd_staging_slot;
}

std::string CSdCard::get_filepath_from_index(std::vector<std::string> scripts, int file_index)
{
    if (file_index < 0 || file_index >= (int)scripts.size())
    {
        printf("CSdCard::get_filename_from_index: invalid file_index: %d\n", file_index);
        return "";
    }

    std::string filepath = "0:Lua/" + scripts[file_index].substr(2) + ".lua"; // substr to remove "U:"

    return filepath;
}

bool CSdCard::remove_lua_extension(char *str)
{
    if (str == NULL)
        return false;

    size_t len = strlen(str);

    if (len < 4 || strcasecmp(str + len - 4, ".lua") != 0)
        return false;

    // Remove extension and limit to LUA_ON_SD_SCRIPT_MAX_FILE_LEN characters
    str[(len - 4 > LUA_ON_SD_SCRIPT_MAX_FILE_LEN) ? LUA_ON_SD_SCRIPT_MAX_FILE_LEN : len - 4] = '\0';

    return true;
}
