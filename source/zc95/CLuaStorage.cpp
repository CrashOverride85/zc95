/*
 * ZC95
 * Copyright (C) 2023  CrashOverride85
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "CLuaStorage.h"
#include "FlashHelper.h"
#include "hardware/flash.h"
#include "pico/flash.h"
#include <string.h>
#include "LuaScripts/LuaScripts.h"
#include "core1/routines/CLuaRoutine.h"

/* CLuaStorage
 * Class to manage the saving of lua scripts to flash, and loading from flash.
 */


CLuaStorage::CLuaStorage()
{
    printf("CLuaStorage::CLuaStorage()\n");
}

CLuaStorage::~CLuaStorage()
{
    printf("CLuaStorage::~CLuaStorage()\n");
}

bool CLuaStorage::is_script_index_valid(uint8_t index)
{
    if (index > lua_script_count())
    {
        printf("CLuaStorage::store_script_section: ERROR - invalid index - %d\n", index);
        return false;
    }

    if (!lua_scripts[index].writeable)
    {
        printf("CLuaStorage::store_script_section: ERROR - script at index %d is not writeable\n", index);
        return false;
    }

    return true;
}

bool CLuaStorage::store_script_section(uint8_t script_index, uint8_t section, const char* lua_script_fragment, size_t buffer_size)
{
    if (!is_script_index_valid(script_index))
        return false;

    if (buffer_size != LUA_UPLOAD_BUFFER_SIZE)
    {
        printf("CLuaStorage::store_script_section: ERROR - expecting buffer to be exactly %d bytes\n", LUA_UPLOAD_BUFFER_SIZE);
        return false;
    }

    uint32_t flash_offset = get_flash_offset(script_index);
    size_t flash_size = get_lua_flash_size(script_index);

    if (flash_size < buffer_size)
    {
        printf("CLuaStorage::store_script_section(): invalid flash size\n");
        return false;
    }

    uint32_t target_offset = flash_offset + (section * buffer_size);
    if (target_offset+buffer_size > flash_offset + flash_size)
    {
        printf("CLuaStorage::store_script_section(): out of flash space in script slot. script_index=%d, flash_offset=%lu, flash_size=%d, target_offset=%lu, section=%d\n",
            script_index, flash_offset, flash_size, target_offset, section);
        return false;
    }

    printf("CLuaStorage::store_script_section(): script_index=%d, script {flash_offset=%lu, flash_size=%d}, section {target_offset=%lu, size=%d, section#=%d}\n",
        script_index, flash_offset, flash_size, target_offset, buffer_size, section);

    flash_write_params_t flash_params = 
    {
        .flash_offset = target_offset,
        .flash_size = buffer_size,
        .buffer_size = buffer_size,
        .lua_script = lua_script_fragment
    };

    return (flash_safe_execute(CLuaStorage::s_do_flash_erase_and_write, &flash_params, UINT32_MAX) == PICO_OK);
}

void CLuaStorage::s_do_flash_erase_and_write(void *param)
{
    const flash_write_params_t *params = (const flash_write_params_t *)param;

    flash_range_erase(params->flash_offset, params->flash_size);

    if (params->lua_script != NULL && params->buffer_size != 0)
        flash_range_program(params->flash_offset, (const uint8_t*)(params->lua_script), params->flash_size);
}

bool CLuaStorage::delete_script_at_index(uint8_t index)
{
    if (!is_script_index_valid(index))
        return false;

    uint32_t flash_offset = get_flash_offset(index);
    size_t flash_size = get_lua_flash_size(index);

    if (flash_offset == 0 || flash_size == 0)
        return false;

    flash_write_params_t flash_params = 
    {
        .flash_offset = flash_offset,
        .flash_size = flash_size,
        .buffer_size = 0,
        .lua_script = NULL
    };

    return (flash_safe_execute(CLuaStorage::s_do_flash_erase_and_write, &flash_params, UINT32_MAX) == PICO_OK);
}

uint32_t CLuaStorage::get_flash_offset(uint8_t script_index)
{
    if (script_index > lua_script_count())
    {
        printf("CLuaStorage::get_flash_offset(): Passed invalid lua script index: %d\n", script_index);
        return 0;
    }

    return lua_scripts[script_index].start - XIP_BASE;
}

size_t CLuaStorage::get_lua_flash_size(uint8_t index)
{
    if (index > lua_script_count())
    {
        printf("CLuaStorage::get_lua_flash_size: Passed invalid lua script index: %d\n", index);
        return 0;
    }

    return lua_scripts[index].end - lua_scripts[index].start;
}

const char* CLuaStorage::get_script_at_index(uint8_t index)
{
    uint32_t flash_offset = get_flash_offset(index);
    if (flash_offset == 0)
        return NULL;

    size_t flash_size = get_lua_flash_size(index);
    if (flash_size == 0)
        return NULL;

    const char *script = (const char *)(flash_offset + XIP_BASE);

    size_t script_length = strnlen(script, flash_size);
    printf("CLuaStorage::get_script_at_index(%d): script size = %d\n", index, script_length);
    if (script_length == 0 || script_length == flash_size)
        return NULL;

    return script;
}

std::list<CLuaStorage::lua_script_t> CLuaStorage::get_lua_scripts(bool writeable_only)
{
    std::list<CLuaStorage::lua_script_t> lua_scripts;

    for (uint8_t index=0; index < lua_script_count(); index++)
    {
        if (writeable_only && !lua_script_is_writable(index))
        {
            printf("get_lua_scripts: skipping %d as not writeable\n", index);
            continue;
        }

        CLuaStorage::lua_script_t lua_script;
        lua_script.index = index;
        lua_script.empty = true;
        lua_script.valid = false;
        lua_script.name  = "<invalid>";

        const char* script = get_script_at_index(index);
        if (script == NULL)
        {
            lua_script.name  = "<empty>";
        }
        else
        {
            lua_script.empty = false;

            CLuaRoutine *lua = new CLuaRoutine(index);
            
            if (lua->is_script_valid())
            {
                struct routine_conf conf;
                lua->get_config(&conf);
                lua_script.name = conf.name;
                lua_script.valid = true;
            }

            delete lua;
        }

        printf("CLuaStorage::get_lua_scripts(): Adding: i=%d, name = %s\n", lua_script.index, lua_script.name.c_str());
        lua_scripts.push_front(lua_script);
    }

    return lua_scripts;
}
