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
#include "config/CMenuSettings.h"
#include "../globals.h"
#include "../core1/output/CFullChannelAsSimpleChannel.h"
#include "../core1/CRoutineOutput.h"


#include <string.h>

CMenuRoutineSelection::CMenuRoutineSelection(
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
    ) : _routines(routines)
{
    printf("CMenuRoutineSelection() \n");
    _display = display;
    _area = display->get_display_area();
    _routine_display_list = new COptionsList(display, _area);
    _buttons = buttons;
    _submenu_active = NULL;
    _settings = settings;
    _hwCheck = hwCheck;
    _routine_output = routine_output;
    _audio = audio;
    _analogueCapture = analogueCapture;
    _wifi = wifi;
    _bluetooth = bluetooth;
    _radio = radio;

    _populate_pattern_list = true;
    populate_routine_list();
}

CMenuRoutineSelection::~CMenuRoutineSelection()
{
    printf("~CMenuRoutineSelection() \n");

    if (_routine_display_list != NULL)
    {
        delete _routine_display_list;
        _routine_display_list = NULL;
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
        if (button == Button::A || button == Button::ROT ) // Select
        {
            uint8_t routine_id = _routine_display_list->get_current_selection_id();
            CRoutines::Routine routine = _routines[routine_id];
            _last_selection = _routine_display_list->get_current_selection();
            set_active_menu(new CMenuRoutineAdjust(_display, routine, _buttons, _routine_output, _audio, _bluetooth, _settings));
            _routine_output->activate_routine(routine_id);
        }

        if (button == Button::B) // "Config"
        {
            set_active_menu(new CMenuSettings(_display, _buttons, _settings, _routine_output, _hwCheck, _audio, _analogueCapture, _wifi, _routines, _bluetooth, _radio));
            _populate_pattern_list = true; // If remote access has been used - accessed via the config menu - the pattern list may have changed
        }
        
        if (button == Button::C) // "Up"
        {
            _routine_display_list->up();
        }

        if (button == Button::D) // "Down"
        {
            _routine_display_list->down();
        }
    }
}

void CMenuRoutineSelection::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
        _submenu_active->adjust_rotary_encoder_change(change);
    else
    {
        if (change >= 1)
        {
            _routine_display_list->down();
        }
        else if (change <= -1)
        {
            _routine_display_list->up();
        }
    }
}

void CMenuRoutineSelection::draw()
{
    _routine_display_list->draw();
}

void CMenuRoutineSelection::show()
{
    _display->set_option_a("Select");
    _display->set_option_b("Config");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    if (_populate_pattern_list)
    {
        populate_routine_list();
        _populate_pattern_list = false;
    }
    
    // If we've already been in a routine and come back to this menu, pre-select that routine, instead
    // of going back to the top of the list
    if (_last_selection > 0)
    {
        _routine_display_list->set_selected(_last_selection);
    }
}

// If a routine has any menu item that uses audio, return true
bool CMenuRoutineSelection::is_audio_routine(routine_conf conf)
{
    return (conf.audio_processing_mode != audio_mode_t::OFF);
}

void CMenuRoutineSelection::populate_routine_list()
{
    // Get a list of routines to show
    _routine_display_list->clear_options();
    int index=0;
    for (std::vector<CRoutines::Routine>::iterator it = _routines.begin(); it != _routines.end(); it++)
    {
        struct routine_conf conf;
        CRoutine* routine = (*it).routine_maker((*it).param);
        routine->get_config(&conf);

        // Add a warning for routines that are able to disable channel isolation
        std::string name;
        if (conf.force_channel_isolation)
            name = conf.name;
        else
            name = "(!)" + conf.name;

        if (!conf.hidden_from_menu)
        {
            // Hide audio routines from menu if audio hardware not present. Show everything else.        
            if (!is_audio_routine(conf))
            {
                _routine_display_list->add_option(name, index);
            }
            else
            {
                if (_audio->get_audio_hardware_state() != audio_hardware_state_t::NOT_PRESENT)
                {
                    _routine_display_list->add_option(name, index);
                }
            }
        }
        index++;
        delete routine;
    }
}
