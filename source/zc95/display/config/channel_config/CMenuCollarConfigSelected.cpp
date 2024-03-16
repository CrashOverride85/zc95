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

#include "CMenuCollarConfigSelected.h"
#include "../config.h"
#include "../core1/output/collar/CCollarComms.h" // for CCollarComms::mode_to_string() 
#include "../core1/CRoutineOutput.h"

CMenuCollarConfigSelected::CMenuCollarConfigSelected(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, uint8_t collar_id, CRoutineOutput *routine_output)
{
    printf("CMenuCollarConfigSelected() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _collar_id = collar_id;
    _routine_output = routine_output;

    display_area disp_area = _display->get_display_area();
    disp_area.y0 += 20;
    _options_list = new COptionsList(display, disp_area);
    _selected_item = 0;
}

CMenuCollarConfigSelected::~CMenuCollarConfigSelected()
{
    printf("~CMenuCollarConfigSelected() \n");
    if (_options_list)
    {
        delete _options_list;
        _options_list = NULL;
    }
}

void CMenuCollarConfigSelected::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::A: // Regenerate / toggle / test 
                button_a_pressed();
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C: // "Up"
                _options_list->up();
                set_button_a_text();
                break;

            case Button::D: // "Down"
                _options_list->down();
                set_button_a_text();
                break;
        }

        _selected_item = _options_list->get_current_selection();
    }
}

void CMenuCollarConfigSelected::button_a_pressed()
{
    switch (_options_list->get_current_selection())
    {
        case setting_id::COLLAR_RF_ID:
            //set_new_collar_rf_code();
            _collar_conf.id = rand() & 0xFFFF;
            _saved_settings->set_collar_config(_collar_id, _collar_conf);
            break;

        case setting_id::COLLAR_CHANNEL:
            if (++_collar_conf.channel > 2) // Channel is saved as 0-2 (as per RF transmission), but displayed (as per the remote) as 1-3
                _collar_conf.channel = 0;
            _saved_settings->set_collar_config(_collar_id, _collar_conf);
            break;

        case setting_id::COLLAR_MODE:
            if (++_collar_conf.mode > 3)
                _collar_conf.mode = 1;
            _saved_settings->set_collar_config(_collar_id, _collar_conf);
            break;

        case setting_id::COLLAR_TEST:
            collar_test();
            break;
        
        default:
            break;
    }
    show();
}

void CMenuCollarConfigSelected::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
}

void CMenuCollarConfigSelected::draw()
{
    display_area disp_area = _display->get_display_area();
    _display->put_text("Collar " + std::to_string(_collar_id+1) + " config", disp_area.x0+2, disp_area.y0 + 10, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));

    _options_list->draw();
}

void CMenuCollarConfigSelected::show()
{
    _display->set_option_a("Select");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    
    if (!_saved_settings->get_collar_config(_collar_id, _collar_conf))
        return;
 
    _options_list->clear_options();
    _options_list->add_option("ID   : " + get_collar_rf_id());
    _options_list->add_option("Chan.: " + std::to_string(_collar_conf.channel+1));
    _options_list->add_option("Mode : " + CCollarComms::mode_to_string(_collar_conf.mode));
    _options_list->add_option("TEST");
    
    _options_list->set_selected(_selected_item);

    set_button_a_text();

    _exit_menu = false;
}

std::string CMenuCollarConfigSelected::get_collar_rf_id()
{
    char buf[10] = {0};
    sprintf(buf, "0x%04x", _collar_conf.id);
    return std::string(buf);
}

void CMenuCollarConfigSelected::set_button_a_text()
{
    switch (_options_list->get_current_selection())
    {
        case setting_id::COLLAR_CHANNEL:
            _display->set_option_a("Toggle");
            break;

        case setting_id::COLLAR_MODE:
            _display->set_option_a("Toggle");
            break;

        case setting_id::COLLAR_RF_ID:
            _display->set_option_a("Regenerate");
            break;

        case setting_id::COLLAR_TEST:
            _display->set_option_a("Test");
            break;
        
        default:
            _display->set_option_a("");
            break;
    }
}

void CMenuCollarConfigSelected::collar_test()
{
    _routine_output->collar_transmit(_collar_conf.id, (CCollarComms::collar_channel)_collar_conf.channel, (CCollarComms::collar_mode)_collar_conf.mode, 1);
}
