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

#include "CMenuRoutineAdjust.h"


CMenuRoutineAdjust::CMenuRoutineAdjust(CDisplay* display, CRoutineMaker* routine_maker, CGetButtonState *buttons, CRoutineOutput *routine_output, CAudio *audio)
{
    printf("CMenuRoutineAdjust() \n");
    struct display_area area;
    _display = display;
    _buttons = buttons;
    _exit_menu = false;
    _area = display->get_display_area();
    _audio = audio;

    _routine_output = routine_output;

    // get routine config
    CRoutine* routine = routine_maker();
    routine->get_config(&_active_routine_conf);
    delete routine;

    // Use the top half of the display for the list routine paramers that can be adjusted
    area = _area;
    area.y1 = area.y1/2;
    _routine_adjust_display_list = new COptionsList(_display, area);
    
    // Use the bottom half of the display for the options list for the selected routine parameter (if it's a multi-choice parameter)
    area = _area;
    area.y0 = area.y1/2;
    area.x0 += 1;
    _routine_multi_choice_list = new COptionsList(_display, area);

    // Set the text used on the status bar
    _title = _active_routine_conf.name;

    enable_audio_if_required_by_routine();
}

CMenuRoutineAdjust::~CMenuRoutineAdjust()
{
    printf("~CMenuRoutineAdjust() \n");
    if (_routine_adjust_display_list)
    {
        delete _routine_adjust_display_list;
        _routine_adjust_display_list = NULL;
    }

    if (_routine_multi_choice_list)
    {
        delete _routine_multi_choice_list;
        _routine_multi_choice_list = NULL;
    }

    _routine_output->stop_routine();
    _audio->set_audio_mode(CAudio::audio_mode_t::OFF);
}

void CMenuRoutineAdjust::button_released(Button button)
{
    if (button == Button::A)
    {
        _routine_output->soft_button_pressed(soft_button::BUTTON_A, false);
    }
}

void CMenuRoutineAdjust::button_pressed(Button button)
{
    // "A" button is passed onto routines, that may or may not use it

    if (button == Button::A)
    {
        _routine_output->soft_button_pressed(soft_button::BUTTON_A, true);
    }

    if (button == Button::B) // "Back"
    {
       _exit_menu = true;
    }

    if (button == Button::C) // "Up"
    {
        _routine_adjust_display_list->up();
        set_options_on_multi_choice_list();
    }

    if (button == Button::D) // "Down"
    {
        _routine_adjust_display_list->down();
        set_options_on_multi_choice_list();
    }
}

void CMenuRoutineAdjust::adjust_rotary_encoder_change(int8_t change)
{
    int32_t new_current_val = 0;
    struct menu_entry *menu_item = &(_active_routine_conf.menu[_routine_adjust_display_list->get_current_selection()]);

    switch (menu_item->menu_type)
    {
        case menu_entry_type::MIN_MAX:
            new_current_val = menu_item->minmax.current_value + (change *  menu_item->minmax.increment_step);

            if (new_current_val > menu_item->minmax.max)
                new_current_val = menu_item->minmax.max;

            if (new_current_val < menu_item->minmax.min)
                new_current_val = menu_item->minmax.min;

            menu_item->minmax.current_value = new_current_val;
            _routine_output->menu_min_max_change(menu_item->id, menu_item->minmax.current_value);
            break;

        case menu_entry_type::MULTI_CHOICE:
            if (change >= 1)
            {
                _routine_multi_choice_list->down();
            }
            else if (change <= -1)
            {
                _routine_multi_choice_list->up();
            }
            else
                break;

            menu_item->multichoice.current_selection = _routine_multi_choice_list->get_current_selection();

            _routine_output->menu_multi_choice_change(menu_item->id, menu_item->multichoice.choices[_routine_multi_choice_list->get_current_selection()].choice_id);
            break;

        case menu_entry_type::AUDIO_VIEW:
            if (change >= 1)
            {
                _audio->increment_trigger_point();
            }
            else if (change <= -1)
            {
                _audio->decrement_trigger_point();
            }

            break;
    }
}

void CMenuRoutineAdjust::draw()
{
    // Show the parameter selection list at the top
    _routine_adjust_display_list->draw();

    // Show menu entry
    uint8_t current_selection = _routine_adjust_display_list->get_current_selection();

    struct menu_entry menu_item = _active_routine_conf.menu[current_selection];

    switch (menu_item.menu_type)
    {
        case menu_entry_type::MIN_MAX:
        {
            color_t bar_colour = hagl_color(0x00, 0x00, 0xFF);
                            
            draw_horz_bar_graph(_area.x0+1, _area.y1*0.66, _area.x1-_area.x0-10, 20, menu_item.minmax.min, menu_item.minmax.max, menu_item.minmax.current_value, menu_item.minmax.UoM, bar_colour);
            break;
        }

        case menu_entry_type::MULTI_CHOICE:
        {
            color_t rect_colour = hagl_color(0x00, 0x00, 0xFF);

            hagl_fill_rectangle(_area.x0+1, _area.y0 + (((_area.y1-_area.y0)/3) * 2) - 11,
                                _area.x1-3, _area.y0 + (((_area.y1-_area.y0)/3) * 2) + 21,
                                    rect_colour);

            _routine_multi_choice_list->draw();
            break;
        }

        case menu_entry_type::AUDIO_VIEW:
        {
            uint8_t x0 = _area.x0+1;
            uint8_t y0 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) - 11;
            uint8_t x1 = _area.x1-3;
            uint8_t y1 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) + 21;

            _audio->draw_audio_view(x0, y0, x1, y1);

            break;
        }
    }
}

void CMenuRoutineAdjust::show()
{
    _display->set_option_a(_active_routine_conf.button_text[(int)soft_button::BUTTON_A]);
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _exit_menu = false;

    _routine_adjust_display_list->clear_options();

    for (std::vector<menu_entry>::iterator it = _active_routine_conf.menu.begin(); it != _active_routine_conf.menu.end(); it++)
    {
        _routine_adjust_display_list->add_option(it->title);
    }

    set_options_on_multi_choice_list();
}

void CMenuRoutineAdjust::draw_horz_bar_graph(int16_t x, int16_t y, uint8_t width, uint8_t height, int16_t min_val, int16_t max_val, int16_t current_val, std::string UoM, color_t bar_colour)
{
    if (current_val > max_val)
        current_val = max_val;
    if (current_val < min_val)
        current_val = min_val;

    if (max_val == 0 || min_val > max_val || max_val-min_val == 0)
        return;


    hagl_draw_rectangle(x, y, x+width, y+height, hagl_color(0xFF, 0xFF, 0xFF));

    float current_val_pc = (((float)current_val - (float)min_val) / ((float)max_val-(float)min_val));
    int16_t bar_width = current_val_pc * (float)(width-1);
  
    hagl_fill_rectangle(x+1, y+1, x+bar_width, y+height-1, bar_colour);

    // min label
    _display->put_text(std::to_string(min_val), x, y + height+3, hagl_color(0, 0xFF, 0));

    // max label
    _display->put_text(std::to_string(max_val), x+width-20, y + height+3, hagl_color(0, 0xFF, 0));

    // current value label in middle of bar
    std::string value_str = std::to_string(current_val) + " " + UoM;
    uint16_t current_val_width = value_str.length() * _display->get_font_width();
    _display->put_text(value_str, x+((width/2)-(current_val_width/2)), y + 1 + (_display->get_font_height()/2), hagl_color(0xFF, 0xFF, 0xFF));
}

void CMenuRoutineAdjust::set_options_on_multi_choice_list()
{
    struct menu_entry selected = _active_routine_conf.menu[_routine_adjust_display_list->get_current_selection()];
    
    if (selected.menu_type == menu_entry_type::MULTI_CHOICE)
    {
        _routine_multi_choice_list->clear_options();

        for (std::vector<multi_choice_option>::iterator it = selected.multichoice.choices.begin(); it != selected.multichoice.choices.end(); it++)
        {
            _routine_multi_choice_list->add_option(it->choice_name);
        }

        _routine_multi_choice_list->set_selected(choice_id_to_menu_index(selected, selected.multichoice.current_selection));
    }
}

// convert a choice id (whatever id is associated with an option) to a menu option (0 indexed menu item)
uint8_t CMenuRoutineAdjust::choice_id_to_menu_index(struct menu_entry selected_menu, uint8_t choice_id)
{    
    for (size_t selected_choice_index = 0; selected_choice_index < selected_menu.multichoice.choices.size(); selected_choice_index++)
    {
        if (selected_menu.multichoice.choices[selected_choice_index].choice_id == choice_id)
        {
            return selected_choice_index;
        }
    }

    printf("CMenuRoutineAdjust::choice_id_to_menu_index(): Invalid config for menu [%s]\n", selected_menu.title.c_str());
    return 0;
}

void CMenuRoutineAdjust::enable_audio_if_required_by_routine()
{
    for (std::vector<menu_entry>::iterator it = _active_routine_conf.menu.begin(); it != _active_routine_conf.menu.end(); it++)
    {
        if (it->menu_type == menu_entry_type::AUDIO_VIEW)
        {
            _audio->set_audio_mode(CAudio::audio_mode_t::THRESHOLD_CROSS_FFT);
            return;
        }
    }
}
