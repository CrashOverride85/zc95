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

#include "CMenuSettingPowerStep.h"

/* 
 * TODO: remove me? This is for a version of the front panel with 5x enoders instead of 1x encoder + 4x POTs
 */

CMenuSettingPowerStep::CMenuSettingPowerStep(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings)
{
    printf("CMenuSettingPowerStep() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;

    

    display_area disp_area = _display->get_display_area();
    struct display_area options_area;
    options_area.x0 = disp_area.x0;
    options_area.x1 = disp_area.x1 - 2;
    options_area.y0 = disp_area.y0 + 20;
    options_area.y1 = disp_area.y1;

    _option_list = new COptionsList(display, options_area);

    _power_step = _saved_settings->get_power_step_interval();
}

CMenuSettingPowerStep::~CMenuSettingPowerStep()
{
    printf("~CMenuSettingPowerStep() \n");
    if (_option_list)
    {
        delete _option_list;
        _option_list = NULL;
    }
}

void CMenuSettingPowerStep::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::B: // "Back"
                _exit_menu = true;
                _saved_settings->save();
                break;

            case Button::C: // "Up"
                _option_list->up();
                update_setting();
                break;

            case Button::D: // "Down"
                _option_list->down();
                update_setting();
                break;

            default:
                break;
        }
    }
}

void CMenuSettingPowerStep::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change >= 1)
        {
            _option_list->down();
        }
        else if (change <= -1)
        {
            _option_list->up();
        }
        
        update_setting();
    }
}

void CMenuSettingPowerStep::update_setting()
{
    _saved_settings->set_power_step_interval(_power_level_steps[_option_list->get_current_selection()].step_interval);
}

void CMenuSettingPowerStep::draw()
{
    display_area disp_area = _display->get_display_area();
    _display->put_text("Power levels", disp_area.x0+2, disp_area.y0 + 10, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));

    _option_list->draw();
}

void CMenuSettingPowerStep::show()
{
    _display->set_option_a(" ");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _power_level_steps.push_back(get_power_level_step("10"  , 100));
    _power_level_steps.push_back(get_power_level_step("25"  ,  40));
    _power_level_steps.push_back(get_power_level_step("50"  ,  20));
    _power_level_steps.push_back(get_power_level_step("100 (default)",10));
    _power_level_steps.push_back(get_power_level_step("250" ,   4));
    _power_level_steps.push_back(get_power_level_step("500" ,   2));
    _power_level_steps.push_back(get_power_level_step("1000",   1));
    
    _option_list->clear_options();
    uint8_t current_setting=0;

    // Now add the possible options to the display list, and select the currently saved option
    for (uint8_t n=0; n < _power_level_steps.size(); n++)
    {
        _option_list->add_option(_power_level_steps[n].text);

        if (_power_level_steps[n].step_interval == _power_step)
        {
            current_setting = n;
        }
    }
    _option_list->set_selected(current_setting);

    _exit_menu = false;
}

CMenuSettingPowerStep::power_level_step CMenuSettingPowerStep::get_power_level_step(std::string text, uint8_t step_interval)
{
    CMenuSettingPowerStep::power_level_step level;
    level.step_interval = step_interval;
    level.text = text;
    return level;
}
