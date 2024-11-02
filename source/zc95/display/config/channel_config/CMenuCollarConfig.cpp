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

#include "CMenuCollarConfig.h"
#include "CMenuCollarConfigSelected.h"
#include "../config.h"
#include "../core1/output/collar/CCollarComms.h"

CMenuCollarConfig::CMenuCollarConfig(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CRoutineOutput *routine_output)
{
    printf("CMenuCollarConfig() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _routine_output = routine_output;

    display_area disp_area = _display->get_display_area();
    disp_area.y0 += 20;
    _collar_list = new COptionsList(display, disp_area);
    _selected_item = 0;
}

CMenuCollarConfig::~CMenuCollarConfig()
{
    printf("~CMenuCollarConfig() \n");
}

void CMenuCollarConfig::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::A: // "Select"
            case Button::ROT:
                set_active_menu(new CMenuCollarConfigSelected(_display, _buttons, _saved_settings, _collar_list->get_current_selection(), _routine_output));
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C: // "Up"
                _collar_list->up();
                break;

            case Button::D: // "Down"
                _collar_list->down();
                break;
        }
        _selected_item = _collar_list->get_current_selection();
    }
}

void CMenuCollarConfig::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change>=1)
        {
            _collar_list->down();
        }
        else if (change<=1)
        {
            _collar_list->up();
        }
    }
}

void CMenuCollarConfig::draw()
{
    display_area disp_area = _display->get_display_area();
    _display->put_text("Collar config", disp_area.x0+2, disp_area.y0 + 10, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));

    _collar_list->draw();
}

void CMenuCollarConfig::show()
{
    _display->set_option_a("Select");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    uint8_t max_collar_id = MAX_CHANNELS-1;
    if (max_collar_id >= EEPROM_CHANNEL_COUNT)
        max_collar_id = 9;

    _collar_list->clear_options();
    for (uint8_t collar_id=0; collar_id <= max_collar_id; collar_id++)
    {
        _collar_list->add_option(get_collar_text(collar_id));
    }
    _collar_list->set_selected(_selected_item);

    _exit_menu = false;
}

std::string CMenuCollarConfig::get_collar_text(uint8_t collar_id)
{
    CSavedSettings::collar_config conf;
    if (_saved_settings->get_collar_config(collar_id, conf))
    {
        return "Collar " + std::to_string(collar_id+1) + ": " + CCollarComms::mode_to_string(conf.mode);
    }

    return "CollarError";
}

