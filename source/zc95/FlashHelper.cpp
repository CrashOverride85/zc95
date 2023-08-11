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

#include "pico/flash.h"
#include "hardware/flash.h"
#include "FlashHelper.h"

/*
 * Custom implementation of flash_safety_helper_t/get_flash_safety_helper() 
 * which messages core1 to suspend it (and waits for that to happen), then 
 * after the flash write has been done resumes everything.
 * 
 * Deals with
 *  - suspending core1
 *  - stopping analogue capture (uses DMA and IRQs)
 *  - saving/disabling interrupts
 * 
 * After write complete, undoes the above.
 * This assumes all flash write are done from Core0 - significant work would 
 * be needed to make it work from core1.
*/

extern mutex_t g_core1_suspend_mutex;
extern struct semaphore g_core1_suspend_sem;

static CAnalogueCapture *_analogue_capture = NULL;
static CRoutineOutput *_routine_output = NULL;
static uint32_t interrupt_state = 0;
static bool _analogue_capture_running = false;

void flash_helper_init(CAnalogueCapture *analogue_capture, CRoutineOutput *routine_output)
{
    _analogue_capture = analogue_capture;
    _routine_output = routine_output;
}

static bool core_init_deinit(bool init) 
{
    return true;
}

static int enter_safe_zone_timeout_ms(uint32_t timeout_ms) 
{
    printf("enter_safe_zone_timeout_ms()\n");

    if (!_analogue_capture || !_routine_output)
    {
        printf("enter_safe_zone_timeout_ms: ERROR - _analogue_capture or _routine_output NULL\n");
        return PICO_ERROR_NOT_PERMITTED;
    }

    // Right now, only core0 will be doing flash writes, so if we're not being 
    // called from core0, something's gone horribly wrong.
    if (get_core_num() != 0)
    {
        printf("enter_safe_zone_timeout_ms: ERROR - not called from core0\n");
        return PICO_ERROR_NOT_PERMITTED;
    }

    mutex_enter_blocking(&g_core1_suspend_mutex);
    sem_acquire_blocking(&g_core1_suspend_sem);

    _routine_output->suspend_core1(); // should release g_core1_suspend_sem once suspended
    sem_acquire_blocking(&g_core1_suspend_sem);

    _analogue_capture_running = _analogue_capture->is_running();
    if (_analogue_capture_running)
    {
        _analogue_capture->stop(); // The DMA done by CAnalogueCapture thoroughly breaks flash writing
    }

    interrupt_state = save_and_disable_interrupts();

    return PICO_OK;
}

static int exit_safe_zone_timeout_ms(uint32_t timeout_ms) 
{
    printf("exit_safe_zone_timeout_ms()\n");

    // restore everything
    restore_interrupts(interrupt_state);

    mutex_exit(&g_core1_suspend_mutex);
    sem_release(&g_core1_suspend_sem);

    printf("flash write done\n");
    if (_analogue_capture_running)
    {
        _analogue_capture->start();
    }

    return PICO_OK;
}

static flash_safety_helper_t flash_safety_helper = 
{
    .core_init_deinit = core_init_deinit,
    .enter_safe_zone_timeout_ms = enter_safe_zone_timeout_ms,
    .exit_safe_zone_timeout_ms = exit_safe_zone_timeout_ms
};

flash_safety_helper_t *get_flash_safety_helper(void) 
{
    return &flash_safety_helper;
}
