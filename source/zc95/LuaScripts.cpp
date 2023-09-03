#include "LuaScripts.h"

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
