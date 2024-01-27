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

#include "CMenuChannelConfig.h"


#include "CMenuSettings.h"
#include "../config.h"


CMenuChannelConfig::CMenuChannelConfig(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings, CRoutineOutput *routine_output)
{
    printf("CMenuChannelConfig() \n");
    _display = display;
    _buttons = buttons;
    _saved_settings = saved_settings;
    _routine_output = routine_output;

    _exit_menu = false;
    display_area area = display->get_display_area();
    area.y1 = area.y0 + ((area.y1-area.y0)/2);
    _channel_list = new COptionsList(display, area);

    _channel_choice_area = display->get_display_area();
    _channel_choice_area.y0 = _channel_choice_area.y1 - ((_channel_choice_area.y1-_channel_choice_area.y0)/2);
    _channel_choices_list = new COptionsList(display, _channel_choice_area);
}

CMenuChannelConfig::~CMenuChannelConfig()
{
    printf("~CMenuChannelConfig() \n");

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_channel_list)
    {
        delete _channel_list;
        _channel_list = NULL;
    }

    if (_channel_choices_list)
    {
        delete _channel_choices_list;
        _channel_choices_list = NULL;
    }

    _routine_output->reinit_channels();
}

void CMenuChannelConfig::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        if (button == Button::B) // "Back"
        {
            _saved_settings->save();
            _exit_menu = true;
        }

        if (button == Button::C) // "Up"
        {
            _channel_list->up();
            set_options_on_multi_choice_list(_channel_list->get_current_selection());
        }

        if (button == Button::D) // "Down"
        {
            _channel_list->down();
            set_options_on_multi_choice_list(_channel_list->get_current_selection());
        }
    }
}

void CMenuChannelConfig::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change >= 1)
        {
            _channel_choices_list->down();
        }
        else if (change <= -1)
        {
            _channel_choices_list->up();
        }
        
       save_selected_channel(_channel_list->get_current_selection(), _channel_choices[_channel_choices_list->get_current_selection()]);
    }
}

void CMenuChannelConfig::save_selected_channel(uint8_t channel_id, channel_choice choice)
{
    CSavedSettings::channel_selection selection;
    selection.type = choice.type;
    selection.index = choice.channel_index;

    _saved_settings->set_channel(selection, channel_id);
}

void CMenuChannelConfig::draw()
{
    _channel_list->draw();

    hagl_color_t rect_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);

    hagl_fill_rectangle(_display->get_hagl_backed(),_channel_choice_area.x0, _channel_choice_area.y0,
                        _channel_choice_area.x1, _channel_choice_area.y1,
                        rect_colour);

    _channel_choices_list->draw();
}


void CMenuChannelConfig::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _channel_list->clear_options();
    for (int n=0; n < MAX_CHANNELS; n++)
    {
        _channel_list->add_option("Channel " + std::to_string(n+1));
    }

    set_options_on_multi_choice_list(0);
    _exit_menu = false;
}

// Channel id is 0-4
void CMenuChannelConfig::set_options_on_multi_choice_list(uint8_t channel_id)
{
    _channel_choices.clear();

    // There are 4 internal channels
    if (channel_id < 4)
    {
        _channel_choices.push_back(get_channel_choice(CChannel_types::channel_type::CHANNEL_INTERNAL, channel_id));
    }

    // I guess in theory there can be any number of linked collars
    _channel_choices.push_back(get_channel_choice(CChannel_types::channel_type::CHANNEL_COLLAR, channel_id));
    
    /*
    // This is to allow the control of an mk312 via the serial port. Pretty pointless now I think. TODO: remove me.
    if (channel_id < 2)
    {
        _channel_choices.push_back(get_channel_choice(CChannel_types::channel_type::CHANNEL_312, channel_id));
    }
    */

    // None is always an option...
    _channel_choices.push_back(get_channel_choice(CChannel_types::channel_type::CHANNEL_NONE, channel_id));

    // Now add the possible options to the display list, and select the currently saved option
    uint8_t current_setting = 0;
    CSavedSettings::channel_selection saved = _saved_settings->get_channel(channel_id);

   _channel_choices_list->clear_options();
    for (uint8_t n=0; n < _channel_choices.size(); n++)
    {
        _channel_choices_list->add_option(_channel_choices[n].text);

        if ((_channel_choices[n].type          == saved.type) && 
            (_channel_choices[n].channel_index == saved.index))
        {
            current_setting = n;
        }
    }
    _channel_choices_list->set_selected(current_setting);
}

CMenuChannelConfig::channel_choice CMenuChannelConfig::get_channel_choice(CChannel_types::channel_type type, uint8_t index)
{
    CMenuChannelConfig::channel_choice cc;
    cc.channel_index = index;
    cc.type = type;
    cc.text = CChannel_types::get_channel_name(type, index);
    return cc;
}


