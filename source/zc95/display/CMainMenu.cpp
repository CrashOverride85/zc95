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

#include "CMainMenu.h"

#include "CMenuRoutineSelection.h"

CMainMenu::CMainMenu(
    CDisplay* display, 
    std::vector<CRoutines::Routine> &routines,
    CGetButtonState *buttons,
    CSavedSettings *settings, 
    CRoutineOutput *routine_output,
    CHwCheck *hwCheck,
    CAudio *audio,
    CAnalogueCapture *analogueCapture,
    CWifi *wifi,
    CBluetooth *bluetooth,
    CRadio *radio
)
{
    printf("CMainMenu() \n");
    _display = display;
    _routines = routines;
    _buttons = buttons;
    _settings = settings;
    _bluetooth = bluetooth;
    _radio = radio;

    _submenu_active = new CMenuRoutineSelection(_display, _routines, _buttons, _settings, routine_output, hwCheck, audio, analogueCapture, wifi, bluetooth, _radio);
    _submenu_active->show();
}

CMainMenu::~CMainMenu()
{
    printf("~CMainMenu() \n");
    if (_submenu_active != NULL)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }
}

void CMainMenu::show()
{
    
}

void CMainMenu::draw()
{


}

void CMainMenu::button_released(Button button)
{
    if (_submenu_active)
        _submenu_active->button_released(button);
}

void CMainMenu::button_pressed(Button button)
{
    if (_submenu_active)
        _submenu_active->button_pressed(button);
}

void CMainMenu::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}
