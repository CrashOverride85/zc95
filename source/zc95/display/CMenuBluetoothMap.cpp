/*
 * ZC95
 * Copyright (C) 2023  CrashOverride85
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

#include "CMenuBluetoothMap.h"
#include "../CDebugOutput.h"
#include "../config.h"


CMenuBluetoothMap::CMenuBluetoothMap(CDisplay* display, CSavedSettings *saved_settings, CBluetooth *bluetooth)
{
    printf("CMenuBluetoothMap()\n");
    _display = display;
    _saved_settings = saved_settings;
    _bluetooth = bluetooth;

    _exit_menu = false;
    display_area area = display->get_display_area();
    area.y1 = area.y0 + ((area.y1-area.y0)/2);
    _keypress_list = new COptionsList(display, area);

    _setting_choice_area = display->get_display_area();
    _setting_choice_area.y0 = _setting_choice_area.y1 - ((_setting_choice_area.y1-_setting_choice_area.y0)/2);
    _keypress_action_list = new COptionsList(display, _setting_choice_area);
}

CMenuBluetoothMap::~CMenuBluetoothMap()
{
    printf("~CMenuBluetoothMap()\n");

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_keypress_list)
    {
        delete _keypress_list;
        _keypress_list = NULL;
    }

    if (_keypress_action_list)
    {
        delete _keypress_action_list;
        _keypress_action_list = NULL;
    }
}

void CMenuBluetoothMap::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        if (button == Button::A)
        {
          /*  if (get_current_setting().id ==  CMenuBluetoothMap::setting_id::SCAN)
            {
             //   set_active_menu(new CMenuBluetoothMapScan(_display, _saved_settings, _bluetooth));
            }
            else if (get_current_setting().id ==  CMenuBluetoothMap::setting_id::TEST)
            {
              //  set_active_menu(new CMenuBluetoothMapTest(_display, _bluetooth));
            } */
        }

        if (button == Button::B) // "Back"
        {
            _exit_menu = true;
        }

        if (button == Button::C) // "Up"
        {
            _keypress_list->up();
            set_options_on_multi_choice_list(_keypress_list->get_current_selection());
        }

        if (button == Button::D) // "Down"
        {
            _keypress_list->down();
            set_options_on_multi_choice_list(_keypress_list->get_current_selection());
        }
    }
}

void CMenuBluetoothMap::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change >= 1)
        {
            _keypress_action_list->down();
        }
        else if (change <= -1)
        {
            _keypress_action_list->up();
        }

        save_setting(_keypress_list->get_current_selection(), _keypress_action_list->get_current_selection());
    }
}

void CMenuBluetoothMap::save_setting(uint8_t setting_menu_index, uint8_t choice_menu_index)
{
    setting_t setting = _settings[setting_menu_index];
    setting_t choice_id = _setting_choices[choice_menu_index];

    switch (setting.id)
    {
     /*   case setting_id::ENABLED:
            _saved_settings->set_bluethooth_enabled(choice_id.id);
            break;
            */
    }
}

void CMenuBluetoothMap::draw()
{
    _keypress_list->draw();
    bool choice_list_required = false;

    setting_t setting = get_current_setting();
    switch (setting.id)
    {
   /*     case setting_id::ENABLED:
            _display->set_option_a(" ");
            choice_list_required = true;
            break;

        case setting_id::SCAN:
            _display->set_option_a("Select");
            choice_list_required = false;
            // TODO
            break;
            */
    }

    // Show list of options for the selected setting in blue box at bottom of screen

    hagl_color_t rect_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);
    hagl_fill_rectangle(_display->get_hagl_backed(), _setting_choice_area.x0, _setting_choice_area.y0,
                        _setting_choice_area.x1, _setting_choice_area.y1, rect_colour);
    _keypress_action_list->draw();

}

void CMenuBluetoothMap::show()
{
    _display->set_option_a(" ");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _settings.clear();
    _settings.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_SHUTTER, "Shutter"));
    _settings.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_BUTTON , "Button" ));
    _settings.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_UP     , "Up"     ));
    _settings.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_DOWN   , "Down"   ));
    _settings.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_LEFT   , "Left"   ));
    _settings.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_RIGHT  , "Right"  ));
    _settings.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_t::KEY_UNKNOWN, "Unknown"));

    _keypress_list->clear_options();
    for (std::vector<CMenuBluetoothMap::setting_t>::iterator it = _settings.begin(); it != _settings.end(); it++)
    {
        _keypress_list->add_option((*it).text);
    }

    _exit_menu = false;
    set_options_on_multi_choice_list(0);
}

void CMenuBluetoothMap::set_options_on_multi_choice_list(uint8_t setting_id)
{
    _setting_choices.clear();
    uint8_t current_choice_id = 0;

    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::NONE      , "<none>"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::BUT_A     , "Top left soft"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::BUT_B     , "Bottom left soft"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::BUT_C     , "Top right soft"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::BUT_D     , "Bottom right soft"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::ROT_LEFT  , "Adjust left"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::ROT_RIGHT , "Adjust right"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::TRIGGER1_A, "Trigger1-A"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::TRIGGER1_B, "Trigger1-B"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::TRIGGER2_A, "Trigger2-A"));
    _setting_choices.push_back(CMenuBluetoothMap::setting_t(CBluetoothRemote::keypress_action_t::TRIGGER2_B, "Trigger2-B"));



    // Add the possible options to the display list, and select the currently saved option
    uint8_t current_setting = 0;
   _keypress_action_list->clear_options();
    for (uint8_t n=0; n < _setting_choices.size(); n++)
    {
        _keypress_action_list->add_option(_setting_choices[n].text);
  //      if (_setting_choices[n].id == current_choice_id)
  //      {
  //          current_setting = n;
  //      }
    }
//    if (_setting_choices.size() > 0)
//        _keypress_action_list->set_selected(current_setting);

}

CMenuBluetoothMap::setting_t CMenuBluetoothMap::get_current_setting()
{
    return _settings[_keypress_list->get_current_selection()];
}
