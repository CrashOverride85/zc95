#ifndef _CLUASTORAGE_H
#define _CLUASTORAGE_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <list>
#include "core1/CRoutineOutput.h"
#include "CAnalogueCapture.h"

extern mutex_t g_core1_suspend_mutex;
extern struct semaphore g_core1_suspend_sem;

class CLuaStorage
{
    public:
        CLuaStorage(CAnalogueCapture *analogue_capture, CRoutineOutput *routine_output);
        ~CLuaStorage();

        struct lua_script_t
        {
            uint8_t index;
            bool empty;
            bool valid;
            std::string name;
        };

        static size_t get_lua_flash_size(uint8_t index);
        bool store_script(uint8_t index, const char* lua_script, size_t buffer_size);
        bool delete_script_at_index(uint8_t index);

        static const char* get_script_at_index(uint8_t index);
        static std::list<CLuaStorage::lua_script_t> get_lua_scripts(bool writeable_only = true);

    private:
        static uint32_t get_flash_offset(uint8_t script_index);
        
        CAnalogueCapture *_analogue_capture;
        CRoutineOutput *_routine_output;

};

#endif
