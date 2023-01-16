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
#include "hardware/flash.h"
#include <string.h>

#include "core1/routines/CLuaRoutine.h"

/* CLuaStorage
 * Class to manage the saving of lua scripts to flash, and loading from flash.
 */

// These are definied in LuaScripts.S and are empty contiguous blocks of 24k flash that can be used 
// to put lua scripts in
extern uint8_t lua_script1_start;  
extern uint8_t lua_script2_start;
extern uint8_t lua_script3_start;
extern uint8_t lua_script4_start;
extern uint8_t lua_script5_start;
extern uint8_t lua_script5_end;

CLuaStorage::CLuaStorage(CAnalogueCapture *analogue_capture, CRoutineOutput *routine_output)
{
    printf("CLuaStorage::CLuaStorage()\n");
    _routine_output = routine_output;
    _analogue_capture = analogue_capture;
}

CLuaStorage::~CLuaStorage()
{
    printf("CLuaStorage::~CLuaStorage()\n");
}

// Store a script. If lua_script == NULL and buffer_size=0, any existing script at specified index is erased without writing anything new
bool CLuaStorage::store_script(uint8_t index, const char* lua_script, size_t buffer_size)
{
    if (!_routine_output)
    {
        // This shouldn't happen...
        printf("CLuaStorage::store_script(): ERROR - _routine_output is NULL\n");
        return false;
    }

    uint32_t flash_offset = get_flash_offset(index);
    size_t flash_size = get_lua_flash_size(index);
    printf("CLuaStorage::store_script: index=%d, flash_offset=%lu, flash_size=%d\n", index, flash_offset, flash_size);
    if (lua_script == NULL && buffer_size == 0)
    {
        printf("Erasing flash for index %d only\n", index);
    }

    if (flash_size == 0)
    {
        // Could do extra validation here. e.g. not too big, is a multiple of 4096, etc.
        printf("CLuaStorage::store_script(): invalid flash size\n");
        return false;
    }

    if 
    (
        buffer_size != flash_size &&
        (!(buffer_size == 0 && lua_script == NULL))
    )
    {
        printf("CLuaStorage::store_script(): ERROR - expected either buffer=0 and script=NULL, or buffer to be %d (but was %d)\n", flash_size, buffer_size);
        return false;
    }    

    mutex_enter_blocking(&g_core1_suspend_mutex);
    sem_acquire_blocking(&g_core1_suspend_sem);

    _routine_output->suspend_core1(); // should release g_core1_suspend_sem once suspended
    sem_acquire_blocking(&g_core1_suspend_sem);
    
    _analogue_capture->stop(); // The DMA done by CAnalogueCapture thoroughly breaks flash writing
    uint32_t save = save_and_disable_interrupts();

    // core1 suspended: write flash
    flash_range_erase(flash_offset, flash_size);

    if (lua_script != NULL && buffer_size != 0)
        flash_range_program(flash_offset, (const uint8_t*)lua_script, flash_size);

    // restore everything
    restore_interrupts(save);
    sleep_ms(1000);

    mutex_exit(&g_core1_suspend_mutex);
    sem_release(&g_core1_suspend_sem);

    printf("flash write done, resume analogue capture\n");
    _analogue_capture->start();
    return true;
}

bool CLuaStorage::delete_script_at_index(uint8_t index)
{
    return store_script(index, NULL, 0);
}

uint32_t CLuaStorage::get_flash_offset(uint8_t script_index)
{
    uint32_t flash_offset = 0;

    switch (script_index)
    {
        case 1:
            flash_offset = ((uint32_t)&lua_script1_start) - XIP_BASE;
            break;

        case 2:
            flash_offset = ((uint32_t)&lua_script2_start) - XIP_BASE;
            break;

        case 3:
            flash_offset = ((uint32_t)&lua_script3_start) - XIP_BASE;
            break;

        case 4:
            flash_offset = ((uint32_t)&lua_script4_start) - XIP_BASE;
            break;

        case 5:
            flash_offset = ((uint32_t)&lua_script5_start) - XIP_BASE;
            break;

        default:
            printf("CLuaStorage::get_flash_offset(): Passed invalid lua script index: %d\n", script_index);
            flash_offset = 0;
            break;
    }

    return flash_offset;
}

size_t CLuaStorage::get_lua_flash_size(uint8_t index)
{
    size_t flash_size = 0;

    switch (index)
    {
        case 1:
            flash_size = (uint32_t)&lua_script2_start - (uint32_t)&lua_script1_start;
            break;

        case 2:
            flash_size = (uint32_t)&lua_script3_start - (uint32_t)&lua_script2_start;
            break;

        case 3:
            flash_size = (uint32_t)&lua_script4_start - (uint32_t)&lua_script3_start;
            break;

        case 4:
            flash_size = (uint32_t)&lua_script5_start - (uint32_t)&lua_script4_start;
            break;

        case 5:
            flash_size = (uint32_t)&lua_script5_end   - (uint32_t)&lua_script5_start;
            break;

        default:
            printf("CLuaStorage::get_lua_flash_size: Passed invalid lua script index: %d\n", index);
            flash_size = 0;
            break;
    }

    return flash_size;
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

std::list<CLuaStorage::lua_script_t> CLuaStorage::get_lua_scripts()
{
    std::list<CLuaStorage::lua_script_t> lua_scripts;

    for (uint8_t index=1; index <= 5; index++)
    {
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
