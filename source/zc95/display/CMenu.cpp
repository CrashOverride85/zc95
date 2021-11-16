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

#include "CMenu.h"

CMenu::~CMenu()
{
    printf("~CMenu()\n");
    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }
}

void CMenu::set_active_menu(CMenu *new_submenu)
{
    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    _submenu_active = new_submenu;
    _submenu_active->show();
}

CMenu::action CMenu::update()
{
    if (_submenu_active)
    {
        if (_submenu_active->update() == CMenu::action::BACK)
        {
            delete _submenu_active;
            _submenu_active = NULL;
            show();
        }
    }
    else
    {
        if (_exit_menu)
            return CMenu::action::BACK;
        draw();
    }

    return CMenu::action::NONE;
}
