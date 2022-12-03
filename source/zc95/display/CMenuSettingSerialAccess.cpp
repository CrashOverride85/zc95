/*
 * ZC95
 * Copyright (C) 2021  CrashOverride85
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

#include "hardware/flash.h"

#include "CMenuSettingSerialAccess.h"
#include "../core1/CRoutineOutput.h"

CMenuSettingSerialAccess::CMenuSettingSerialAccess(CDisplay* display, CGetButtonState *buttons, CRoutineOutput *routine_output, CAnalogueCapture *analogueCapture)
{
    printf("CMenuSettingSerialAccess() \n");
    _display = display;
    _buttons = buttons;
    _routine_output = routine_output;
    _analogueCapture = analogueCapture;
}

CMenuSettingSerialAccess::~CMenuSettingSerialAccess()
{
    printf("~CMenuSettingSerialAccess() \n");
}

void CMenuSettingSerialAccess::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::A:
                do_update();
                break; 

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            default:
                break;
        }
    }
}


void CMenuSettingSerialAccess::do_update()
{
    mutex_enter_blocking(&g_core1_suspend_mutex);
    sem_acquire_blocking(&g_core1_suspend_sem);

    _routine_output->suspend_core1(); // should release g_core1_suspend_sem once suspended
    sem_acquire_blocking(&g_core1_suspend_sem);
    
    _analogueCapture->stop(); // The DMA done by CAnalogueCapture thoroughly breaks flash writing
    uint32_t save = save_and_disable_interrupts();

    // core1 suspended: do stuff
    write_flash();

    restore_interrupts(save);
    sleep_ms(1000);

    mutex_exit(&g_core1_suspend_mutex);
    sem_release(&g_core1_suspend_sem);

    printf("done, resume analogue capture\n");
    _analogueCapture->start();
}

void CMenuSettingSerialAccess::write_flash()
{
    extern uint8_t lua_script1_start;  
    extern uint8_t lua_script2_start;


    uint32_t flash_offset = ((uint32_t)&lua_script1_start) - XIP_BASE;
    printf("flash_offset     = %d\n", flash_offset);
    
    flash_range_erase(flash_offset, (uint32_t)(&lua_script2_start-&lua_script1_start));
    
    printf("flash erase done\n");
}

void CMenuSettingSerialAccess::adjust_rotary_encoder_change(int8_t change)
{

}

void CMenuSettingSerialAccess::draw()
{
    
}

void CMenuSettingSerialAccess::put_text_line(std::string text, int16_t x, int16_t y, uint8_t line, color_t colour)
{
    _display->put_text(text, x, y + (line * 10), colour);
}

void CMenuSettingSerialAccess::show()
{
    _display->set_option_a("FLASH");
    _display->set_option_b("Back");
    _display->set_option_c(" ");
    _display->set_option_d(" ");

    _exit_menu = false;
}
