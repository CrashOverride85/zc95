#include "LuaScripts/LuaScripts.h"
#include <string.h>

uint8_t lua_script_count()
{
    const lua_script_in_flash_t *scripts = lua_scripts;
    uint8_t count = 0;

    while  ((scripts++)->start)
        count++;

    return count;
}

bool lua_script_is_writable(uint8_t index)
{
    if (index >= lua_script_count())
        return false;

    return lua_scripts[index].writeable;
}

int get_lib_index_by_name(const char *name)
{
    const lua_lib_in_flash_t *script = lua_libs;
    uint8_t index = 0;

    while (script->start)
    {
        if (!strcmp(name, script->name))
        {
            return index;
        }
        script++;
        index++;
    }

    return -1;
}

const char *get_lib_by_index(uint8_t index)
{
    return (const char *)(lua_libs[index].start);
}

int get_lib_length_by_index(uint8_t index)
{
    return lua_libs[index].end - lua_libs[index].start;
}
