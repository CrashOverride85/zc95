#ifndef _CSDCARD_H
#define _CSDCARD_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <vector>

#include "../CLuaStorage.h"
#include "../CAnalogueCapture.h"
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
#include "hw_config.h"

class CSdCard
{
    public:
        CSdCard(CAnalogueCapture* analogue_capture);
        ~CSdCard();

        void mount();
        void unmount();
        bool write_file(std::string filename, const char* contents, size_t len);
        bool copy_file_to_flash(std::string script_title); // script title is the script name as it appears in the menu (U: + file name of lua script, minus the .lua extension)

        std::vector<std::string> get_lua_scripts();

    private:
        uint8_t sd_staging_slot();
        std::string get_filepath_from_index(int file_index);
        bool remove_lua_extension(char *str);

        std::string _script_in_flash = ""; // script currently loaded into flash

        CAnalogueCapture* _analogue_capture;
        bool _set_analogue_capture_running_on_unmount = false;
        CLuaStorage *_lua_storage;
        bool _mounted = false;
        
        uint8_t *_file_buffer = NULL;
};

#endif
