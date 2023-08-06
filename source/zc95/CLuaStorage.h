#ifndef _CLUASTORAGE_H
#define _CLUASTORAGE_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <list>

class CLuaStorage
{
    public:
        CLuaStorage();
        ~CLuaStorage();

        struct lua_script_t
        {
            uint8_t index;
            bool empty;
            bool valid;
            std::string name;
        };

        struct flash_write_params_t
        {
            uint32_t flash_offset;
            size_t flash_size;
            size_t buffer_size;
            const char* lua_script;
        };

        static size_t get_lua_flash_size(uint8_t index);
        bool store_script(uint8_t index, const char* lua_script, size_t buffer_size);
        bool delete_script_at_index(uint8_t index);

        static const char* get_script_at_index(uint8_t index);
        static std::list<CLuaStorage::lua_script_t> get_lua_scripts();

        static void s_do_flash_erase_and_write(void *param);

    private:
        static uint32_t get_flash_offset(uint8_t script_index);
};

#endif
