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


CMenuRoutineAdjust::CMenuRoutineAdjust(
                CDisplay* display, 
                CRoutines::Routine routine, 
                CGetButtonState *buttons, 
                CRoutineOutput *routine_output, 
                CAudio *audio, 
                CBluetooth *bluetooth,
                CSavedSettings *saved_settings) 
{
    printf("CMenuRoutineAdjust() \n");
    struct display_area area;
    _display = display;
    _buttons = buttons;
    _exit_menu = false;
    _area = display->get_display_area();
    _audio = audio;
    _bluetooth = bluetooth;
    _saved_settings = saved_settings;
    _routine_output = routine_output;

    // get routine config
    CRoutine* routine_ptr = routine.routine_maker(routine.param);
    routine_ptr->get_config(&_active_routine_conf);
    delete routine_ptr;

    // Bluetooth doesn't work well with analogue capture for audio running, so for now, don't allow bluetooth 
    // and audio at the same time.
    _bt_enabled = _saved_settings->get_bluethooth_enabled() && _active_routine_conf.audio_processing_mode == audio_mode_t::OFF;
    if (_bt_enabled)
    {
        queue_init(&_bt_keypress_queue, sizeof(CBluetoothRemote::bt_keypress_queue_entry_t), 5);
        _bluetooth->set_keypress_queue(&_bt_keypress_queue);
    }

    // Use the top half of the display for the list routine parameters that can be adjusted
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
}

CMenuRoutineAdjust::~CMenuRoutineAdjust()
{
    printf("~CMenuRoutineAdjust()\n");

    if (_bt_enabled)
    {
        _bluetooth->set_keypress_queue(NULL);
        queue_free(&_bt_keypress_queue);
        _bluetooth->set_state(CBluetooth::state_t::OFF);
    }

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
    uint8_t menu_selection = _routine_adjust_display_list->get_current_selection_id();

    // "A" button is passed onto routines, that may or may not use it
    if (button == Button::A)
    {
        _routine_output->soft_button_pressed(soft_button::BUTTON_A, true);
    }

    if (button == Button::B) // "Back"
    {
       _exit_menu = true;
    }

    // Menu up / down
    
    // If there's no options, exit now
    if (_active_routine_conf.menu.size() <= 0)
        return;

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

    // if selected menu has changed, signal the routine (although most won't care)
    if (button == Button::C || button == Button::D) // Up or Down
    {
        if (_routine_adjust_display_list->get_current_selection_id() != menu_selection)
        {
            struct menu_entry *menu_item = &(_active_routine_conf.menu[_routine_adjust_display_list->get_current_selection()]);
            _routine_output->menu_selected(menu_item->id);
        }
    }
}

void CMenuRoutineAdjust::adjust_rotary_encoder_change(int8_t change)
{
    int32_t new_current_val = 0;

    if (_active_routine_conf.menu.size() <= 0)
        return;

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

        case menu_entry_type::AUDIO_VIEW_SPECT:
            if (change >= 1)
            {
                _audio->increment_trigger_point();
            }
            else if (change <= -1)
            {
                _audio->decrement_trigger_point();
            }

        case menu_entry_type::AUDIO_VIEW_WAVE:
        case menu_entry_type::AUDIO_VIEW_INTENSITY_STEREO:
        case menu_entry_type::AUDIO_VIEW_INTENSITY_MONO:
        case menu_entry_type::AUDIO_VIEW_VIRTUAL_3:
            if (change >= 1)
            {
                increment_gain(10);
            }
            else if (change <= -1)
            {
                decrement_gain(10);
            }

            break;
    }
}

void CMenuRoutineAdjust::draw()
{
    if (_routine_output->get_lua_script_state() == lua_script_state_t::INVALID)
    {
        draw_bad_script_screen();
        return;
    }

    // Show the parameter selection list at the top
    _routine_adjust_display_list->draw();

    // If no menu entries (i.e. pattern with nothing configurable), nothing to do
    if (_active_routine_conf.menu.size() <= 0)
        return;

    // Show menu entry
    uint8_t current_selection = _routine_adjust_display_list->get_current_selection();

    struct menu_entry menu_item = _active_routine_conf.menu[current_selection];

    switch (menu_item.menu_type)
    {
        case menu_entry_type::MIN_MAX:
        {
            hagl_color_t bar_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);
                            
            draw_horz_bar_graph(_area.x0+1, _area.y1*0.66, _area.x1-_area.x0-10, 20, menu_item.minmax.min, menu_item.minmax.max, menu_item.minmax.current_value, menu_item.minmax.UoM, bar_colour);
            break;
        }

        case menu_entry_type::MULTI_CHOICE:
        {
            hagl_color_t rect_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);

            hagl_fill_rectangle(_display->get_hagl_backed(), _area.x0+1, _area.y0 + (((_area.y1-_area.y0)/3) * 2) - 11,
                                _area.x1-3, _area.y0 + (((_area.y1-_area.y0)/3) * 2) + 21,
                                    rect_colour);

            _routine_multi_choice_list->draw();
            break;
        }

        case menu_entry_type::AUDIO_VIEW_SPECT:
        {
            uint8_t x0 = _area.x0+1;
            uint8_t y0 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) - 11;
            uint8_t x1 = _area.x1-3;
            uint8_t y1 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) + 21;

            _audio->draw_audio_view(x0, y0, x1, y1);

            break;
        }

        case menu_entry_type::AUDIO_VIEW_WAVE:
        {
            uint8_t x0 = _area.x0+1;
            uint8_t y0 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) - 11;
            uint8_t x1 = _area.x1-3;
            uint8_t y1 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) + 21;

            _audio->draw_audio_wave(x0, y0, x1, y1, true, false);

            break;
        }

        case menu_entry_type::AUDIO_VIEW_INTENSITY_MONO:
        {
            uint8_t x0 = _area.x0+1;
            uint8_t y0 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) - 11;
            uint8_t x1 = _area.x1-3;
            uint8_t y1 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) + 21;

            _audio->draw_audio_wave(x0, y0, x1, y1, true, true);

            break;
        }

        case menu_entry_type::AUDIO_VIEW_INTENSITY_STEREO:
        {
            uint8_t x0 = _area.x0+1;
            uint8_t y0 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) - 11;
            uint8_t x1 = _area.x1-3;
            uint8_t y1 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) + 21;

            _audio->draw_audio_wave(x0, y0, x1, y1, true, false);

            break;
        }

        case menu_entry_type::AUDIO_VIEW_VIRTUAL_3:
        {
            uint8_t x0 = _area.x0+1;
            uint8_t y0 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) - 11;
            uint8_t x1 = _area.x1-3;
            uint8_t y1 = _area.y0 + (((_area.y1-_area.y0)/3) * 2) + 21;

            _audio->draw_audio_virt3(x0, y0, x1, y1, true);

            break;
        }
    }

    // Process input from bluetooth remote, if enabled
    if (_bt_enabled)
    {
        CBluetoothRemote::bt_keypress_queue_entry_t queue_entry;
        while (queue_try_remove(&_bt_keypress_queue, &queue_entry))
        {
            if (_active_routine_conf.bluetooth_remote_passthrough)
                _routine_output->bluetooth_remote_passthrough(queue_entry.key);
            else
                process_bluetooth_remote_keypress(queue_entry.key);
        }
    }
}

void CMenuRoutineAdjust::process_bluetooth_remote_keypress(CBluetoothRemote::keypress_t key)
{
    CBluetoothRemote::keypress_action_t action = _saved_settings->get_bt_keypress_action(key);
    switch (action)
    {
        case CBluetoothRemote::keypress_action_t::BUT_A:
            button_pressed(Button::A);
            button_released(Button::A);
            break;

        case CBluetoothRemote::keypress_action_t::BUT_B:
            button_pressed(Button::B);
            button_released(Button::B);
            break;

        case CBluetoothRemote::keypress_action_t::BUT_C:
            button_pressed(Button::C);
            button_released(Button::C);
            break;

        case CBluetoothRemote::keypress_action_t::BUT_D:
            button_pressed(Button::D);
            button_released(Button::D);
            break;

        case CBluetoothRemote::keypress_action_t::ROT_LEFT:
            adjust_rotary_encoder_change(-1);
            break;

        case CBluetoothRemote::keypress_action_t::ROT_RIGHT:
            adjust_rotary_encoder_change(1);
            break;

        case CBluetoothRemote::keypress_action_t::TRIGGER1_A:
            _routine_output->trigger(trigger_socket::Trigger1, trigger_part::A, true);
            _routine_output->trigger(trigger_socket::Trigger1, trigger_part::A, false);
            break;

        case CBluetoothRemote::keypress_action_t::TRIGGER1_B:
            _routine_output->trigger(trigger_socket::Trigger1, trigger_part::B, true);
            _routine_output->trigger(trigger_socket::Trigger1, trigger_part::B, false);
            break;

        case CBluetoothRemote::keypress_action_t::TRIGGER2_A:
            _routine_output->trigger(trigger_socket::Trigger2, trigger_part::A, true);
            _routine_output->trigger(trigger_socket::Trigger2, trigger_part::A, false);
            break;

        case CBluetoothRemote::keypress_action_t::TRIGGER2_B:
            _routine_output->trigger(trigger_socket::Trigger2, trigger_part::B, true);
            _routine_output->trigger(trigger_socket::Trigger2, trigger_part::B, false);
            break;

        case CBluetoothRemote::keypress_action_t::NONE:
            printf("CMenuRoutineAdjust: No configured action for [%s]\n", CBluetoothRemote::s_get_keypress_string(key).c_str());
            break;
        
        default:
            // If this is reached, there's a bug somewhere
            printf("CMenuRoutineAdjust: Unexpected bluetooth action: 0x%X\n", key);
            break;
    }
}

void CMenuRoutineAdjust::draw_bad_script_screen()
{
    hagl_color_t colour = hagl_color(_display->get_hagl_backed(), 0xFF, 0x00, 0x00);

    hagl_draw_line(_display->get_hagl_backed(), _area.x0, _area.y0, _area.x1, _area.y1, colour);
    hagl_draw_line(_display->get_hagl_backed(), _area.x1, _area.y0, _area.x0, _area.y1, colour);

    _display->put_text("Script error", 0, _area.y0 + ((_area.y1 - _area.y0) / 2), hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0x00));
}

void CMenuRoutineAdjust::show()
{
    _display->set_option_a(_active_routine_conf.button_text[(int)soft_button::BUTTON_A]);
    _display->set_option_b("Back");

    if (_active_routine_conf.menu.size() > 1)
    {
        // Only show Up/Down soft button labels if there's more than one parameter available to change
        _display->set_option_c("Up");
        _display->set_option_d("Down");
    }
    else
    {
        _display->set_option_c("");
        _display->set_option_d("");
    }
    _exit_menu = false;

    _routine_adjust_display_list->clear_options();

    for (std::vector<menu_entry>::iterator it = _active_routine_conf.menu.begin(); it != _active_routine_conf.menu.end(); it++)
    {
        _routine_adjust_display_list->add_option(it->title);
    }

    set_options_on_multi_choice_list();

    if (_bt_enabled)
    {
        bd_addr_t paired_addr = {0};
        CSavedSettings::bt_device_type_t bt_type = _saved_settings->get_paired_bt_type();
        _saved_settings->get_paired_bt_address(&paired_addr);
        _bluetooth->connect(paired_addr, bt_type);
    }
}

void CMenuRoutineAdjust::draw_horz_bar_graph(int16_t x, int16_t y, uint8_t width, uint8_t height, int16_t min_val, int16_t max_val, int16_t current_val, std::string UoM, hagl_color_t bar_colour)
{
    if (current_val > max_val)
        current_val = max_val;
    if (current_val < min_val)
        current_val = min_val;

    if (max_val == 0 || min_val > max_val || max_val-min_val == 0)
        return;


    hagl_draw_rectangle(_display->get_hagl_backed(), x, y, x+width, y+height, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));

    float current_val_pc = (((float)current_val - (float)min_val) / ((float)max_val-(float)min_val));
    int16_t bar_width = current_val_pc * (float)(width-1);
  
    hagl_fill_rectangle(_display->get_hagl_backed(), x+1, y+1, x+bar_width, y+height-1, bar_colour);

    // min label
    _display->put_text(std::to_string(min_val), x, y + height+3, hagl_color(_display->get_hagl_backed(), 0, 0xFF, 0));

    // max label
    _display->put_text(std::to_string(max_val), x+width-20, y + height+3, hagl_color(_display->get_hagl_backed(), 0, 0xFF, 0));

    // current value label in middle of bar
    std::string value_str = std::to_string(current_val) + " " + UoM;
    uint16_t current_val_width = value_str.length() * _display->get_font_width();
    _display->put_text(value_str, x+((width/2)-(current_val_width/2)), y + 1 + (_display->get_font_height()/2), hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
}

void CMenuRoutineAdjust::set_options_on_multi_choice_list()
{
    if (_active_routine_conf.menu.size() <= 0)
        return;

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

void CMenuRoutineAdjust::increment_gain(uint8_t by)
{
    uint8_t left  = 0;
    uint8_t right = 0;
    _audio->get_current_gain(&left, &right);

    int16_t new_left  = left;
    int16_t new_right = right;

    new_left  += by;
    new_right += by;

    if (new_left > 255)
        left = 255;
    else
        left += by;

    if (new_right > 255)
        right = 255;
    else
        right += by;

    _audio->set_gain(CAnalogueCapture::channel::LEFT , left );
    _audio->set_gain(CAnalogueCapture::channel::RIGHT, right);
}

void CMenuRoutineAdjust::decrement_gain(uint8_t by)
{
    uint8_t left  = 0;
    uint8_t right = 0;
    _audio->get_current_gain(&left, &right);

    int16_t new_left  = left;
    int16_t new_right = right;

    new_left  -= by;
    new_right -= by;

   if (new_left < 0)
        left = 0;
    else
        left -= by;

    if (new_right < 0)
        right = 0;
    else
        right -= by;

    _audio->set_gain(CAnalogueCapture::channel::LEFT , left );
    _audio->set_gain(CAnalogueCapture::channel::RIGHT, right);
}
