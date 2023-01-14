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

#include "../../external/lua/lua-5.1.5/include/lstate.h"

#include "CLuaRoutine.h"
#include <string>
#include <string.h>

typedef int (CLuaRoutine::*mem_func)(lua_State * L);

// Copied / adpated from https://stackoverflow.com/a/32416597 
template <mem_func func> int dispatch(lua_State *L) 
{
    if (L->l_G->ud)
    {
        CLuaRoutine *ptr = (CLuaRoutine*)(L->l_G->ud);
        return ((*ptr).*func)(L);
    }

    return 0;
}

CLuaRoutine::CLuaRoutine(uint8_t script_index)
{
    printf("CLuaRoutine(%d)\n", script_index);
    _lua_state = NULL;
    _script_valid = ScriptValid::UNKNOWN;

    _lua_state = luaL_newstate_ud(this);
    luaL_openlibs(_lua_state);
    load_lua_script_if_required();
}

CLuaRoutine::~CLuaRoutine()
{
    printf("~CLuaRoutine()\n");
 
    if (_lua_state)
    {
        _lua_state->l_G->ud = NULL;
        lua_close(_lua_state);
        _lua_state = NULL;
    }
}

void CLuaRoutine::load_lua_script_if_required()
{
    if (!_lua_state)
        return;

    if (_script_valid != ScriptValid::UNKNOWN)
        return;

   // printf("CLuaRoutine::load_lua_script_if_required()\n");
// extern uint8_t lua_script1_start;
   // void *original_start2 = &lua_script1_start;

    extern uint8_t arm_payload_start;
    void *original_start = &arm_payload_start;

    if (CheckLua(luaL_dostring(_lua_state, (const char*)original_start)))
    {
        _script_valid = ScriptValid::VALID;

        const luaL_Reg regs[] = {
            { "ChannelOn"     , &dispatch<&CLuaRoutine::lua_channel_on>  },
            { "ChannelOff"    , &dispatch<&CLuaRoutine::lua_channel_off> },
            { "ChannelPulseMs", &dispatch<&CLuaRoutine::lua_channel_pulse_ms> },
            { "SetPower"      , &dispatch<&CLuaRoutine::lua_set_power> },
            { "SetFrequency"  , &dispatch<&CLuaRoutine::lua_set_freq> },
            { "SetPulseWidth" , &dispatch<&CLuaRoutine::lua_set_pulse_width> },
            { NULL, NULL }
        };
        luaL_register(_lua_state, "zc", regs);
    }
    else
    {
        printf("CLuaRoutine: script INVALID\n");
        _script_valid = ScriptValid::INVALID;
        lua_close(_lua_state);
        _lua_state = NULL;
    }
}

bool CLuaRoutine::is_script_valid()
{
    load_lua_script_if_required();
    if (!_lua_state)
        return false;

    return (_script_valid == ScriptValid::VALID);
}

void CLuaRoutine::get_config(struct routine_conf *conf)
{
    load_lua_script_if_required();

    if (!_lua_state)
        return;

    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);
    conf->outputs.push_back(output_type::FULL);

    lua_getglobal(_lua_state, "Config");
    if (lua_istable(_lua_state, -1))
    {
        lua_pushstring(_lua_state, "name");
        lua_gettable(_lua_state, -2);
        conf->name = lua_tostring(_lua_state, -1);
        lua_pop(_lua_state, 1);

        lua_pushstring(_lua_state, "menu_items");
        lua_gettable(_lua_state, -2);

        uint8_t menu_item_count = lua_objlen(_lua_state, -1);

        for (uint8_t i = 1; i <= menu_item_count; ++i)
        {
            struct menu_entry entry;
            printf("\n\n");
            lua_rawgeti(_lua_state, -1, i);

            entry.title = get_string_field("title");
            entry.id = get_int_field("id");
            std::string menu_type_str = get_string_field("type");

            menu_entry_type menu_type;
            if (GetMenuEntryTypeFromString(menu_type_str.c_str(), &menu_type))
            {
                switch (menu_type)
                {
                    case menu_entry_type::MIN_MAX:
                        printf("* MIN_MAX\n");
                        get_min_max_entry(&entry);
                        break;

                    case menu_entry_type::MULTI_CHOICE:
                        printf("* MULTI_CHOICE\n");
                        get_multi_choice_entry(&entry);
                        break;
                }
            }
            conf->menu.push_back(entry);

            lua_pop(_lua_state, 1);
        }

        lua_pop(_lua_state, 1);
    }

    conf->name = "U:" + conf->name;
}

bool CLuaRoutine::runnable()
{
    return (_lua_state && _script_valid == ScriptValid::VALID);
}

void CLuaRoutine::menu_min_max_change(uint8_t menu_id, int16_t new_value) 
{
    load_lua_script_if_required();
    if (!runnable())
        return;

    lua_getglobal(_lua_state, "MinMaxChange");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushinteger(_lua_state, menu_id);
        lua_pushinteger(_lua_state, new_value);
        pcall(2, 0, 0);
    }
}

void CLuaRoutine::menu_multi_choice_change(uint8_t menu_id, uint8_t choice_id)
{
    load_lua_script_if_required();
    if (!runnable())
        return;

    lua_getglobal(_lua_state, "MultiChoiceChange");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushinteger(_lua_state, menu_id);
        lua_pushinteger(_lua_state, choice_id);
        pcall(2, 0, 0);
    }
}

void CLuaRoutine::soft_button_pushed (soft_button button, bool pushed)
{
    load_lua_script_if_required();
    if (!runnable())
        return;

    /*
    if (button == soft_button::BUTTON_A)
    {
        if (_pulse_mode)
        {
            // Pulse mode (pulse for preset length when button pushed)
            if (pushed)
            {
                all_channels_pulse(_pulse_len_ms);
            }
        }
        else
        {
            // Continous mode (on whilst button held down)
            if (pushed)
                all_channels(true);
            else
                all_channels(false);
        }
    }
    */
}

void CLuaRoutine::trigger(trigger_socket socket, trigger_part part, bool active)
{
    load_lua_script_if_required();
    if (!runnable())
        return;

    // External tigger input is the same as pressing the "Fire" soft-button
    //soft_button_pushed(soft_button::BUTTON_A, !active);
}

void CLuaRoutine::start()
{
    load_lua_script_if_required();
    if (!runnable())
        return;

    set_all_channels_power(POWER_FULL);
}

void CLuaRoutine::loop(uint64_t time_us)
{
    channel_pulse_processing();
    load_lua_script_if_required();
    if (!runnable())
        return;

    int32_t time_ms = time_us/1000;
    lua_getglobal(_lua_state, "Loop");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushinteger(_lua_state, time_ms);
        pcall(1, 0, 0);
    }
}

void CLuaRoutine::channel_pulse_processing()
{
    for (uint8_t channel_id = 0; channel_id < CHANNEL_COUNT; channel_id++)
    {
        if (_channel_switch_off_at_us[channel_id])
        {
            if (time_us_64() >  _channel_switch_off_at_us[channel_id])
            {
                full_channel_off(channel_id);
                _channel_switch_off_at_us[channel_id] = 0;
            }
        }
    }
}

void CLuaRoutine::stop()
{
    set_all_channels_power(0);
    for (int x=0; x < CHANNEL_COUNT; x++)    
        full_channel_off(x);
}

//////////////////////////////////// LUA //////////////////////////////////////////////////////

// Call lua in protected mode to trap any errors in the lua script. 
// If anything goes wrong, output the error, mark the script as invalid and switch off the output
int CLuaRoutine::pcall (int nargs, int nresults, int errfunc)
{
    int retval = lua_pcall(_lua_state, nargs, nresults, errfunc);
    if (retval)
    {
        printf("CLuaRoutine::pcall error: %s\n", lua_tostring(_lua_state, -1));
        _script_valid = ScriptValid::INVALID;
        stop();
    }

    return retval;
}

/////////////////////////////////////
///// Processing for get config /////
/////////////////////////////////////

bool CLuaRoutine::GetMenuEntryTypeFromString(const char* type, menu_entry_type *menu_type_out)
{
    if (!strcasecmp(type, "MIN_MAX"))
        *menu_type_out = menu_entry_type::MIN_MAX;
    else if (!strcasecmp(type, "MULTI_CHOICE"))
        *menu_type_out = menu_entry_type::MULTI_CHOICE;    
    else
    {
        printf("Unexpected menu type encountered: %s\n", type);
        return false;
    }

    return true;
}

bool CLuaRoutine::CheckLua(int r)
{
	if (r != 0)
	{
		printf("Error: %s\n", lua_tostring(_lua_state, -1));
		return false;
	}
	return true;
}

void CLuaRoutine::get_multi_choice_entry(struct menu_entry *entry)
{
    lua_pushstring(_lua_state, "choices");
    lua_gettable(_lua_state, -2);

    entry->menu_type = menu_entry_type::MULTI_CHOICE;
    entry->multichoice.current_selection = 1;

    uint8_t menu_item_choice_count = lua_objlen(_lua_state, -1);

    for (uint8_t j = 1; j <= menu_item_choice_count; ++j)
    {
        struct multi_choice_option choice;
        lua_rawgeti(_lua_state, -1, j);

        choice.choice_id   = get_int_field   ("choice_id");
        choice.choice_name = get_string_field("description");

        lua_pop(_lua_state, 1);

        entry->multichoice.choices.push_back(choice);
    }

    lua_pop(_lua_state, 1);
}


void CLuaRoutine::get_min_max_entry(struct menu_entry *entry)
{
    entry->menu_type = menu_entry_type::MIN_MAX;

    entry->minmax.min = get_int_field("min");
    entry->minmax.max = get_int_field("max");
    entry->minmax.increment_step = get_int_field("increment_step");
    entry->minmax.current_value = get_int_field("default");
    entry->minmax.UoM = get_string_field("uom");
}

int CLuaRoutine::get_int_field(const char *field_name)
{
    int number;
    lua_getfield(_lua_state, -1, field_name);
    number = lua_tonumber(_lua_state, -1);
    lua_pop(_lua_state, 1);
    return number;
}

std::string CLuaRoutine::get_string_field(const char *field_name)
{
    std::string str;
    lua_getfield(_lua_state, -1, field_name);
    str = lua_tostring(_lua_state, -1);
    lua_pop(_lua_state, 1);
    return str;
}

bool CLuaRoutine::is_channel_number_valid(int channel_number)
{
    if (channel_number >= 1 && channel_number <= 4)
        return true;
    else
        return false;
}

/////////////////////////////////////
////// Called from Lua scripts //////
/////////////////////////////////////


// Takes one param: channel number (1-4)
int CLuaRoutine::lua_channel_on(lua_State *L)
{
	int chan = lua_tointeger(L, 1);
    if (!is_channel_number_valid(chan)) return 0;

    full_channel_on(chan-1);
    return 1;
}

// Takes one param: channel number (1-4)
int CLuaRoutine::lua_channel_off(lua_State *L)
{
    int chan = lua_tointeger(L, 1);
    if (!is_channel_number_valid(chan)) return 0;

    full_channel_off(chan-1);
    return 1;
}

// Params: 
// int: channel number (1-4)
// int: duration (ms)
int CLuaRoutine::lua_channel_pulse_ms(lua_State *L)
{
    // Channel will be switched off by channel_pulse_processing() which is called from loop()
    int chan        = lua_tointeger(L, 1);
    int duration_ms = lua_tointeger(L, 2);
    if (!is_channel_number_valid(chan)) return 0;
    if (duration_ms < 0) return 0;

    _channel_switch_off_at_us[chan-1] = time_us_64() + (duration_ms * 1000);
    full_channel_on(chan-1);

    return 1;
}

// Params:
// int: channel number (1-4)
// int: power (0-1000)
int CLuaRoutine::lua_set_power(lua_State *L)
{
    int chan = lua_tointeger(L, 1);
    int power = lua_tointeger(L, 2);
    if (!is_channel_number_valid(chan)) return 0;
    if (power <= 0 || power > 1000) return 0;

    full_channel_set_power(chan-1, power);
    return 1;
}

// Params:
// int: channel number (1-4)
// int: frequency (1 - 300) Hz
int CLuaRoutine::lua_set_freq(lua_State *L)
{
    int chan = lua_tointeger(L, 1);
    int freq = lua_tointeger(L, 2);
    if (!is_channel_number_valid(chan)) return 0;
    if (freq <= 0 || freq > 300) return 0;

    full_channel_set_freq(chan-1, freq);
    return 1;
}

// Params:
// int: channel number (1-4)
// int: positive pulse width (1-255) us
// int: negative pulse width (1-255) us
int CLuaRoutine::lua_set_pulse_width(lua_State *L)
{
    int chan = lua_tointeger(L, 1);
    int pos = lua_tointeger(L, 2);
    int neg = lua_tointeger(L, 3);

    if (!is_channel_number_valid(chan)) return 0;
    if (pos <= 0 || pos > 255) return 0;
    if (neg <= 0 || neg > 255) return 0;

    full_channel_set_pulse_width(chan-1, pos, neg);
    return 1;
}
