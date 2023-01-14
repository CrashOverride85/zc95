#include "CLuaStorage.h"
#include "hardware/flash.h"

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

    if (buffer_size != flash_size)
    {
        printf("CLuaStorage::store_script(): ERROR - expected buffer to be %d but was %d\n", flash_size, buffer_size);
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
