/*
 * ZC95
 * Copyright (C) 2022  CrashOverride85
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

#include "CPredicament.h"

enum menu_ids
{
    TRG1_INV = 1,
    TRG2_INV = 2,
    LOGIC_MODE = 3,
    OUTPUT_INV = 4
};

#define CHANNEL_COUNT 4

CPredicament::CPredicament(uint8_t param)
{
    printf("CPredicament()\n");
    _output_on = false;
    _trigger1_active = false;
    _trigger2_active = false;

    // menu options. init value here should match current_selection set below
    _trigger1_invert = false;
    _trigger2_invert = false;
    _logic_and = true;
    _output_invert = true;
}

CPredicament::~CPredicament()
{
    printf("~CPredicament()\n");
}

void CPredicament::config(struct routine_conf *conf)
{
    conf->name = "Predicament";

    // Want 4x simple channels
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);
    conf->outputs.push_back(output_type::SIMPLE);

    // menu entry 1: "Trigger 1 invert"
    struct menu_entry menu_trg1_inv;
    menu_trg1_inv.id = menu_ids::TRG1_INV;
    menu_trg1_inv.title = "Trigger1 invert";
    menu_trg1_inv.menu_type = menu_entry_type::MULTI_CHOICE;
    menu_trg1_inv.multichoice.current_selection = 0;
    menu_trg1_inv.multichoice.choices.push_back(get_choice("Yes", 1));
    menu_trg1_inv.multichoice.choices.push_back(get_choice("No", 0));
    conf->menu.push_back(menu_trg1_inv);

    // menu entry 2: "Trigger 2 invert"
    struct menu_entry menu_trg2_inv;
    menu_trg2_inv.id = menu_ids::TRG2_INV;
    menu_trg2_inv.title = "Trigger2 invert";
    menu_trg2_inv.menu_type = menu_entry_type::MULTI_CHOICE;
    menu_trg2_inv.multichoice.current_selection = 0;
    menu_trg2_inv.multichoice.choices.push_back(get_choice("Yes", 1));
    menu_trg2_inv.multichoice.choices.push_back(get_choice("No", 0));
    conf->menu.push_back(menu_trg2_inv);

    // menu entry 3: "Logic" : and / or
    struct menu_entry menu_logic;
    menu_logic.id = menu_ids::LOGIC_MODE;
    menu_logic.title = "Logic";
    menu_logic.menu_type = menu_entry_type::MULTI_CHOICE;
    menu_logic.multichoice.current_selection = 1;
    menu_logic.multichoice.choices.push_back(get_choice("Or", 0));
    menu_logic.multichoice.choices.push_back(get_choice("And", 1));
    conf->menu.push_back(menu_logic);

    // menu entry 4 "Output invert" : Yes / No
    struct menu_entry menu_output_inv;
    menu_output_inv.id = menu_ids::OUTPUT_INV;
    menu_output_inv.title = "Output invert";
    menu_output_inv.menu_type = menu_entry_type::MULTI_CHOICE;
    menu_output_inv.multichoice.current_selection = 1;
    menu_output_inv.multichoice.choices.push_back(get_choice("Yes", 1));
    menu_output_inv.multichoice.choices.push_back(get_choice("No", 0));
    conf->menu.push_back(menu_output_inv);
}

void CPredicament::get_config(struct routine_conf *conf)
{
   CPredicament::config(conf);
}

void CPredicament::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{

}

void CPredicament::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{    
    switch (menu_id)
    {
        case TRG1_INV:
            _trigger1_invert = (choice_id == 1);
            break;

        case TRG2_INV:
            _trigger2_invert = (choice_id == 1);
            break;

        case LOGIC_MODE:
            _logic_and = (choice_id == 1);
            break;

        case OUTPUT_INV:
            _output_invert = (choice_id == 1);
            break;
    }
}

void CPredicament::soft_button_pushed (soft_button button, bool pushed)
{

}

void CPredicament::trigger(trigger_socket socket, trigger_part part, bool active)
{
    if (socket == trigger_socket::Trigger1 && part == trigger_part::A)
        _trigger1_active = active;

    if (socket == trigger_socket::Trigger2 && part == trigger_part::A)
        _trigger2_active = active; 
}

void CPredicament::start()
{
    set_all_channels_power(POWER_FULL);
}

void CPredicament::loop(uint64_t time_us)
{
    bool new_output_state = get_required_output_state();

    if (new_output_state != _output_on)
    {
        all_channels(new_output_state);
        _output_on = new_output_state;
    }
}

bool CPredicament::get_required_output_state()
{
    bool trigger1 = _trigger1_active;
    bool trigger2 = _trigger2_active;
    if (_trigger1_invert) trigger1 = !trigger1;
    if (_trigger2_invert) trigger2 = !trigger2;

    bool output;
    if (_logic_and)
        output = trigger1 && trigger2;
    else
        output = trigger1 || trigger2;

    if (_output_invert)
        output = !output;

    return output;
}

void CPredicament::stop()
{
   set_all_channels_power(0);
    for (int x=0; x < CHANNEL_COUNT; x++)    
        simple_channel_off(x);
}

void CPredicament::all_channels(bool on)
{
    for (int x=0; x < CHANNEL_COUNT; x++)   
    {
        if (on)
            simple_channel_on(x);
        else
            simple_channel_off(x);
    }
}
