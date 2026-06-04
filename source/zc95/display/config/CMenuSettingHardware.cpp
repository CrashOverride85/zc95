/*
 * ZC95
 * Copyright (C) 2025  CrashOverride85
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

#include "CMenuSettingHardware.h"
#include "CMenuSettings.h"
#include "../CDebugOutput.h"
#include "../../common/zc95_config.h"


CMenuSettingHardware::CMenuSettingHardware(CDisplay* display, CSavedSettings *saved_settings, CRoutineOutput *routine_output, CAudio *audio, IHal* hal)
{
    printf("CMenuSettingHardware()\n");
    _display = display;
    _saved_settings = saved_settings;
    _routine_output = routine_output;
    _hal = hal;

    _exit_menu = false;
    display_area area = display->get_display_area();
    area.y1 = area.y0 + ((area.y1-area.y0)/2);
    _settings_list = new COptionsList(display, area);
    _bar_graph = new CHorzBarGraph(_display);

    _setting_choice_area = display->get_display_area();
    _setting_choice_area.y0 = _setting_choice_area.y1 - ((_setting_choice_area.y1-_setting_choice_area.y0)/2);
    _settings_choice_list = new COptionsList(display, _setting_choice_area);
    _setting_choice_area.x1 -= 2;
    _audio = audio;
}

CMenuSettingHardware::~CMenuSettingHardware()
{
    printf("~CMenuSettingHardware()\n");
    _hal->led_control()->show_rbg_test_pattern(false);

    if (_submenu_active)
    {
        delete _submenu_active;
        _submenu_active = NULL;
    }

    if (_bar_graph)
    {
        delete _bar_graph;
        _bar_graph = NULL;
    }

    if (_settings_list)
    {
        delete _settings_list;
        _settings_list = NULL;
    }

    if (_settings_choice_list)
    {
        delete _settings_choice_list;
        _settings_choice_list = NULL;
    }
}

void CMenuSettingHardware::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        if (button == Button::B) // "Back"
        {
            _exit_menu = true;
        }

        if (button == Button::C) // "Up"
        {
            _settings_list->up();
            set_options_for_selection(_settings_list->get_current_selection_id());
        }

        if (button == Button::D) // "Down"
        {
            _settings_list->down();
            set_options_for_selection(_settings_list->get_current_selection_id());
        }
    }
}

void CMenuSettingHardware::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change >= 1)
        {
            if (_settings_list->get_current_selection_id() == setting_id::CHARGE_CURRENT)
            {
                _charge_current_ma += 60;
                if (_charge_current_ma > 3000)
                    _charge_current_ma = 3000;
            }
            else
            {
                _settings_choice_list->down();
            }
        }
        else if (change <= -1)
        {
            if (_settings_list->get_current_selection_id() == setting_id::CHARGE_CURRENT)
            {
                _charge_current_ma -= 60;
                if (_charge_current_ma < 0)
                    _charge_current_ma = 0;
            }
            else
            {
                _settings_choice_list->up();
            }
        }

        save_setting(_settings_list->get_current_selection_id(), _settings_choice_list->get_current_selection_id());
    }
}

void CMenuSettingHardware::save_setting(uint8_t setting_menu_index, uint8_t choice_id)
{
     _hal->led_control()->show_rbg_test_pattern(false);
    switch (_settings_list->get_current_selection_id())
    {
        case setting_id::AUDIO:
            _saved_settings->set_audio_setting((CSavedSettings::setting_audio)choice_id);
            break;

        case setting_id::DEBUG:
            _saved_settings->set_debug_dest((CSavedSettings::setting_debug)choice_id);
            CDebugOutput::set_debug_destination_from_settings(_saved_settings);
            break;

        case setting_id::AUX_USE:
            _saved_settings->set_aux_port_use((CSavedSettings::setting_aux_port_use)choice_id);
            if (choice_id == (uint8_t)CSavedSettings::setting_aux_port_use::AUDIO)
                _hal->audio_input_enable(true);
            else
                _hal->audio_input_enable(false);
            break;
            
        case setting_id::LED_FORMAT:
            _saved_settings->set_led_colour_format((CSavedSettings::led_colour_format_t)choice_id);
            _hal->led_control()->show_rbg_test_pattern(true);
            break;

        case setting_id::CHARGE_CURRENT:
            _saved_settings->set_batt_charge_current(_charge_current_ma);
            break;
    }
}

void CMenuSettingHardware::draw()
{
    _settings_list->draw();


    if (_settings_list->get_current_selection_id() == setting_id::CHARGE_CURRENT)
    {
        _bar_graph->draw_horz_bar_graph( _setting_choice_area, 0, 3000, _charge_current_ma, "mA", hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF));
    }
    else
    {
        hagl_color_t rect_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);

        hagl_fill_rectangle(_display->get_hagl_backed(), _setting_choice_area.x0, _setting_choice_area.y0,
                            _setting_choice_area.x1, _setting_choice_area.y1, rect_colour);

        _settings_choice_list->draw();
    }
}

void CMenuSettingHardware::show()
{
    _display->set_option_a("");
    _display->set_option_b("Back");
    _display->set_option_c("Up");
    _display->set_option_d("Down");

    _settings_list->clear_options();
    _settings_list->add_option("Audio", setting_id::AUDIO);
    _settings_list->add_option("Debug destination", setting_id::DEBUG);

    // Only MKII's with the later V2.2+ main board are able to set the max charge current.
    if (_hal->hardware_version() == zc95_version_t::MKII && _hal->hardware_variant() == hw_variant_t::V2_2)
    {
        _settings_list->add_option("Batt charge current", setting_id::CHARGE_CURRENT);
    }

    // MKI's have an aux port that is dual use: for audio or serial depending on the AUX_USE setting. 
    // MKII's have separate serial and audio sockets so this setting is meaningless.
    if (_hal->hardware_version() == zc95_version_t::MKI)
        _settings_list->add_option("Aux port use", setting_id::AUX_USE);

    _settings_list->add_option("LED colour format", setting_id::LED_FORMAT);

    _exit_menu = false;
    set_options_for_selection(0);
}

void CMenuSettingHardware::set_options_for_selection(uint8_t setting_id)
{
    _settings_choice_list->clear_options();
    uint8_t current_choice_id = 0;
    std::string port_name = _hal->hardware_version() == zc95_version_t::MKI ? "Aux" : "Serial"; // MKI has single Aux port, MKII has Serial+Audio
    _hal->led_control()->show_rbg_test_pattern(false);

    switch (setting_id)
    {
        case setting_id::AUDIO:
            _settings_choice_list->add_option("Auto"        , (uint8_t)CSavedSettings::setting_audio::AUTO   );
            _settings_choice_list->add_option("On (no gain)", (uint8_t)CSavedSettings::setting_audio::NO_GAIN);
            _settings_choice_list->add_option("Off"         , (uint8_t)CSavedSettings::setting_audio::OFF    );
            current_choice_id = (uint8_t)_saved_settings->get_audio_setting();
            break;

        case setting_id::DEBUG:
            _settings_choice_list->add_option("Accessory port", (uint8_t)CSavedSettings::setting_debug::ACC_PORT);
            _settings_choice_list->add_option(port_name       , (uint8_t)CSavedSettings::setting_debug::AUX_PORT);
            _settings_choice_list->add_option("Off"           , (uint8_t)CSavedSettings::setting_debug::OFF     );
            current_choice_id = (uint8_t)_saved_settings->get_debug_dest();
            break;

        case setting_id::AUX_USE:
            _settings_choice_list->add_option("Audio input", (uint8_t)CSavedSettings::setting_aux_port_use::AUDIO );
            _settings_choice_list->add_option("Serial I/O" , (uint8_t)CSavedSettings::setting_aux_port_use::SERIAL);
            current_choice_id = (uint8_t)_saved_settings->get_aux_port_use();
            break;

        case setting_id::LED_FORMAT:
            _settings_choice_list->add_option("RGB", (uint8_t)CSavedSettings::led_colour_format_t::RGB);    
            _settings_choice_list->add_option("RBG", (uint8_t)CSavedSettings::led_colour_format_t::RBG);
            _settings_choice_list->add_option("BGR", (uint8_t)CSavedSettings::led_colour_format_t::BGR);
            _settings_choice_list->add_option("BRG", (uint8_t)CSavedSettings::led_colour_format_t::BRG);
            _settings_choice_list->add_option("GRB", (uint8_t)CSavedSettings::led_colour_format_t::GRB);
            _settings_choice_list->add_option("GBR", (uint8_t)CSavedSettings::led_colour_format_t::GBR);
            current_choice_id = (uint8_t)_saved_settings->get_led_colour_format();
            _hal->led_control()->show_rbg_test_pattern(true);
            break;

        case setting_id::CHARGE_CURRENT:
            // N/A: showing adjust bar graph for this
            break;
    }

    // Add the possible options to the display list, and select the currently saved option  

    if (setting_id == setting_id::CHARGE_CURRENT)
    {
        _charge_current_ma = _saved_settings->get_batt_charge_current_mA();
    }
    else
    {
        _settings_choice_list->set_selected_by_id(current_choice_id);
    }
}
