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

#include "CMenuRoutineSelection.h"
#include "CMenuRoutineAdjust.h"
#include "CMenuSettings.h"
#include "../core1/output/CFullChannelAsSimpleChannel.h"

#include "../core1/CRoutineOutput.h"


#include <string.h>

CMenuRoutineSelection::CMenuRoutineSelection(
    CDisplay* display, 
    std::vector<CRoutineMaker*> *routines, 
    CGetButtonState *buttons, 
    CSavedSettings *settings, 
    CRoutineOutput *routine_output,
    CHwCheck *hwCheck,
    CAudio *audio)
{
    printf("CMenuRoutineSelection() \n");
    _display = display;
    _area = display->get_display_area();
    _routine_disply_list = new COptionsList(display, _area);
    _routines = routines;
    _buttons = buttons;
    _submenu_active = NULL;
    _settings = settings;
    _hwCheck = hwCheck;
    _routine_output = routine_output;
    _audio = audio;
}

CMenuRoutineSelection::~CMenuRoutineSelection()
{
    printf("~CMenuRoutineSelection() \n");

    if (_routine_disply_list != NULL)
    {
        delete _routine_disply_list;
        _routine_disply_list = NULL;
    }

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }
}

void CMenuRoutineSelection::button_released(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_released(button);
    }
}

void CMenuRoutineSelection::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
        {
        if (button == Button::A) // Select
        {
            CRoutineMaker* routine_maker = (*_routines)[_routine_disply_list->get_current_selection()];
            //_routine_output->activate_routine(routine_maker);
            _routine_output->activate_routine(_routine_disply_list->get_current_selection());
            _last_selection = _routine_disply_list->get_current_selection();

            set_active_menu(new CMenuRoutineAdjust(_display, routine_maker, _buttons, _routine_output));
        }

        if (button == Button::B) // "Config"
        {
            set_active_menu(new CMenuSettings(_display, _buttons, _settings, _routine_output, _hwCheck, _audio));
        }
        
        if (button == Button::C) // "Up"
        {
            _routine_disply_list->up();
        }

        if (button == Button::D) // "Down"
        {
            _routine_disply_list->down();
        }
    }
}

void CMenuRoutineSelection::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
}

void CMenuRoutineSelection::draw()
{
    _routine_disply_list->draw();
}

void CMenuRoutineSelection::show()
{
    _display->set_option_a("Select");
    _display->set_option_b("Config");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    // Get a list of routines to show
    _routine_disply_list->clear_options();
    for (std::vector<CRoutineMaker*>::iterator it = _routines->begin(); it != _routines->end(); it++)
    {
        struct routine_conf conf;
        CRoutine* routine = (*it)();
        routine->get_config(&conf);
        delete routine;
        
        _routine_disply_list->add_option(conf.name);
    }

    // If we've already been in a routine and come back to this menu, pre-select that routine, instead
    // of going back to the top of the list
    if (_last_selection > 0)
    {
        _routine_disply_list->set_selected(_last_selection);
    }
}
