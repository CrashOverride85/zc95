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

#include "../../external/lua/lua-5.1.5/include/lstate.h"

#include "CLuaRoutine.h"
#include "../../config.h"
#include <string>
#include <string.h>

typedef int (CLuaRoutine::*mem_func)(lua_State * L);

// Copied / adapted from https://stackoverflow.com/a/32416597 
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
    _script = NULL;
    _script_index = script_index;
    CLuaRoutine_common();
}

CLuaRoutine::CLuaRoutine(const char *script)
{
    printf("CLuaRoutine(<script>)\n");
    _script = script;
    CLuaRoutine_common();
}

void CLuaRoutine::CLuaRoutine_common()
{
    _last_lua_error = "";
    _lua_state = NULL;
    _script_valid = ScriptValid::UNKNOWN;

    _lua_state = luaL_newstate_ud(this);
    luaL_openlibs(_lua_state);
    lua_sethook(_lua_state, CLuaRoutine::s_lua_hook, LUA_MASKCOUNT, _hook_call_frequency);
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

    const char *script;
    if (_script)
        script = _script;
    else
        script = CLuaStorage::get_script_at_index(_script_index);

    if (script == NULL)
    {
        printf("CLuaRoutine::load_lua_script_if_required(): No or invalid script at index %d\n", _script_index);
        lua_close(_lua_state);
        _lua_state = NULL;
        _script_valid = ScriptValid::INVALID;
        return;
    }

    if (CheckLua(luaL_dostring(_lua_state, script)))
    {
        _script_valid = ScriptValid::VALID;

        const luaL_Reg zc_regs[] = {
            { "ChannelOn"     , &dispatch<&CLuaRoutine::lua_channel_on>  },
            { "ChannelOff"    , &dispatch<&CLuaRoutine::lua_channel_off> },
            { "ChannelPulseMs", &dispatch<&CLuaRoutine::lua_channel_pulse_ms> },
            { "SetPower"      , &dispatch<&CLuaRoutine::lua_set_power> },
            { "SetFrequency"  , &dispatch<&CLuaRoutine::lua_set_freq> },
            { "SetPulseWidth" , &dispatch<&CLuaRoutine::lua_set_pulse_width> },
            { "AccIoWrite"    , &dispatch<&CLuaRoutine::lua_acc_io_write> },
            { NULL, NULL }
        };
        luaL_register(_lua_state, "zc", zc_regs);

        const luaL_Reg global_regs[] = 
        {
            { "print"     , &dispatch<&CLuaRoutine::lua_print>},
            { NULL, NULL }
        };
        lua_getglobal(_lua_state, "_G");
        luaL_register(_lua_state, NULL, global_regs);
        lua_pop(_lua_state, 1);
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
    if (_script_valid == ScriptValid::INVALID)
        return false;

    routine_conf conf;
    if (!get_and_validate_config(&conf))
        return false;

    if (!_lua_state)
        return false;

    return (_script_valid == ScriptValid::VALID);
}

void CLuaRoutine::get_config(struct routine_conf *conf)
{
    get_and_validate_config(conf); 
}

bool CLuaRoutine::get_and_validate_config(struct routine_conf *conf)
{
    bool is_valid = true;
    load_lua_script_if_required();

    if (!_lua_state)
    {
        return false;
    }

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

        conf->button_text[(int)soft_button::BUTTON_A] = get_string_field("soft_button");
        int loop_freq = get_int_field("loop_freq_hz");

        printf("loop_freq = %d\n", loop_freq);
        if (loop_freq < 0 || loop_freq > 400)
        {
            _last_lua_error = "Configured loop_freq_hz of " +  std::to_string(loop_freq) + " is not valid";
            printf("%s\n", _last_lua_error.c_str());
            conf->loop_freq_hz = 0; 
            is_valid = false;
        }
        else
        {
            conf->loop_freq_hz = loop_freq; 
        }

        lua_pushstring(_lua_state, "menu_items");
        lua_gettable(_lua_state, -2);

        uint8_t menu_item_count = lua_objlen(_lua_state, -1);

        for (uint8_t i = 1; i <= menu_item_count; ++i)
        {
            struct menu_entry entry;
            lua_rawgeti(_lua_state, -1, i);

            entry.title = get_string_field("title");
            entry.id = get_int_field("id");
            entry.group_id = get_int_field("group");
            std::string menu_type_str = get_string_field("type");

            menu_entry_type menu_type;
            if (GetMenuEntryTypeFromString(menu_type_str.c_str(), &menu_type))
            {
                switch (menu_type)
                {
                    case menu_entry_type::MIN_MAX:
                        get_min_max_entry(&entry);
                        break;

                    case menu_entry_type::MULTI_CHOICE:
                        get_multi_choice_entry(&entry);
                        break;

                    case menu_entry_type::AUDIO_VIEW_SPECT:
                    case menu_entry_type::AUDIO_VIEW_WAVE:
                    case menu_entry_type::AUDIO_VIEW_INTENSITY_STEREO:
                    case menu_entry_type::AUDIO_VIEW_INTENSITY_MONO:
                    case menu_entry_type::AUDIO_VIEW_VIRTUAL_3:
                        // Unsupported from Lua (so far)
                        // GetMenuEntryTypeFromString() should mean we never end up here.
                        break;
                }
            }
            conf->menu.push_back(entry);

            lua_pop(_lua_state, 1);
        }

        lua_pop(_lua_state, 1);
    }

    conf->name = "U:" + conf->name;


    printf("get_and_validate_config: returning [%d]\n", is_valid);
    return is_valid;
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

    if (button == soft_button::BUTTON_A)
    {
        lua_getglobal(_lua_state, "SoftButton");
        if (lua_isfunction(_lua_state, -1))
        {
            lua_pushboolean(_lua_state, pushed);
            pcall(1, 0, 0);
        }
    }
}

void CLuaRoutine::trigger(trigger_socket socket, trigger_part part, bool active)
{
    load_lua_script_if_required();
    if (!runnable())
        return;

    std::string str_socket;
    std::string str_part;

    switch (socket)
    {
        case trigger_socket::Trigger1:
            str_socket = "TRIGGER1";
            break;

        case trigger_socket::Trigger2:
            str_socket = "TRIGGER2";
            break;

        default:
            printf("CLuaRoutine::trigger: Error, unexpected trigger socket: %d\n", (int)socket);
            return;
    }

    switch (part)
    {
        case trigger_part::A:
            str_part = "A";
            break;

        case trigger_part::B:
            str_part = "B";
            break;

        default:
            printf("CLuaRoutine::trigger: Error, unexpected trigger part: %d\n", (int)part);
            return;
    }

    lua_getglobal(_lua_state, "ExternalTrigger");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushstring(_lua_state, str_socket.c_str());
        lua_pushstring(_lua_state, str_part.c_str());
        lua_pushboolean(_lua_state, active);
        pcall(3, 0, 0);
    }
}

void CLuaRoutine::start()
{
    load_lua_script_if_required();

    routine_conf conf;
    if (!get_and_validate_config(&conf))
        _script_valid = ScriptValid::INVALID;
    _loop_freq_hz = conf.loop_freq_hz;

    if (!runnable())
        return;

    lua_getglobal(_lua_state, "Setup");
    if (lua_isfunction(_lua_state, -1))
    {
        int32_t time_ms = time_us_64()/1000;
        lua_pushinteger(_lua_state, time_ms);
        pcall(1, 0, 0);
    }
    else
    {
        set_all_channels_power(POWER_FULL);
    }
}

void CLuaRoutine::loop(uint64_t time_us)
{
    channel_pulse_processing();
    load_lua_script_if_required();
    if (!runnable())
        return;

    double time_ms = (double)time_us/(double)1000;

    if (_loop_freq_hz)
    {
        uint32_t loop_freq = floor(time_ms/((double)1000/(double)_loop_freq_hz));
        if (loop_freq == _last_loop)
            return;

        _last_loop = loop_freq;
    }

    lua_getglobal(_lua_state, "Loop");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushnumber(_lua_state, time_ms);
        pcall(1, 0, 0);
    }
    else
    {
        // There must be a loop function, or the script isn't going to work
        printf("CLuaRoutine::loop(): No loop function in script!\n");
        print(text_type_t::ERROR, "Script stopped... no loop function found in script!");
        _script_valid = ScriptValid::INVALID;
        stop();
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
    for (int channel_id=0; channel_id < CHANNEL_COUNT; channel_id++)    
    {
        full_channel_off(channel_id);
        _channel_switch_off_at_us[channel_id] = 0;
    }
}

lua_script_state_t CLuaRoutine::lua_script_state()
{
    if (_script_valid == ScriptValid::INVALID)
        return lua_script_state_t::INVALID;
    else
        return lua_script_state_t::VALID;
}

//////////////////////////////////// LUA //////////////////////////////////////////////////////

// Call lua in protected mode to trap any errors in the lua script. 
// If anything goes wrong, output the error, mark the script as invalid and switch off the output
int CLuaRoutine::pcall (int nargs, int nresults, int errfunc)
{
    _instruction_count = 0;
    int retval = lua_pcall(_lua_state, nargs, nresults, errfunc);
    if (retval)
    {
        const char *err = lua_tostring(_lua_state, -1);
        printf("CLuaRoutine::pcall error: %s\n", err);
        print(text_type_t::ERROR, "Script stopped... error: \n%s", err);
        _script_valid = ScriptValid::INVALID;
        stop();
    }

    return retval;
}

void CLuaRoutine::s_lua_hook(lua_State *L, lua_Debug *ar)
{
    CLuaRoutine *ptr = (CLuaRoutine*)(L->l_G->ud);
    if (ptr)
    {
        ptr->lua_hook(ar);
    }
}

// If a Lua function continuously executes for more than about LUA_MAX_INTRUCTIONS, kill the 
// script. This is to protect from infinite loops locking up the box.
void CLuaRoutine::lua_hook(lua_Debug *ar)
{
    if (ar->event == LUA_HOOKCOUNT)
    {
        _instruction_count += _hook_call_frequency;
        if (_instruction_count > LUA_MAX_INSTRUCTIONS)
        {
            printf("CLuaRoutine(): Terminating execution of Lua script due to instruction limit being reached\n");
            luaL_error(_lua_state, "Instruction limit reached");
            return;
        }
    }
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
        const char *lua_error = lua_tostring(_lua_state, -1);
        _last_lua_error = lua_error;
		printf("Error: %s\n", lua_error);
		return false;
	}
	return true;
}

std::string CLuaRoutine::get_last_lua_error()
{
    return _last_lua_error;
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

// Copied from lbaselib.c/luaB_print (lua_State *L) and tweaked
int CLuaRoutine::lua_print(lua_State *L)
{
    std::string output_string = "";
    int n = lua_gettop(L);  /* number of arguments */
    int i;
    lua_getglobal(L, "tostring");
    for (i=1; i<=n; i++) 
    {
        const char *s;
        lua_pushvalue(L, -1);  /* function to be called */
        lua_pushvalue(L, i);   /* value to print */
        lua_call(L, 1, 1);
        s = lua_tostring(L, -1);  /* get result */
        if (s == NULL)
        {
            return luaL_error(L, LUA_QL("tostring") " must return a string to "
                                LUA_QL("print"));
        }
        if (i>1) 
            output_string += "\t";

        output_string += s;
        lua_pop(L, 1);  /* pop result */
    }
    printf("[LUA] %s\n", output_string.c_str());
    print(text_type_t::PRINT, "%s", output_string.c_str());
    return 0;
}

// Takes one param: channel number (1-4)
int CLuaRoutine::lua_channel_on(lua_State *L)
{
	int chan = lua_tointeger(L, 1);
    if (!is_channel_number_valid(chan)) return 0;

    full_channel_on(chan-1);
    _channel_switch_off_at_us[chan-1] = 0;
    return 1;
}

// Takes one param: channel number (1-4)
int CLuaRoutine::lua_channel_off(lua_State *L)
{
    int chan = lua_tointeger(L, 1);
    if (!is_channel_number_valid(chan)) return 0;

    full_channel_off(chan-1);
    _channel_switch_off_at_us[chan-1] = 0;
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
    if (power < 0 || power > 1000) return 0;

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
// int: positive pulse width (0-255) us
// int: negative pulse width (0-255) us
int CLuaRoutine::lua_set_pulse_width(lua_State *L)
{
    int chan = lua_tointeger(L, 1);
    int pos = lua_tointeger(L, 2);
    int neg = lua_tointeger(L, 3);

    if (!is_channel_number_valid(chan)) return 0;
    if (pos < 0 || pos > 255) return 0;
    if (neg < 0 || neg > 255) return 0;

    full_channel_set_pulse_width(chan-1, pos, neg);
    return 1;
}

// Params:
// int : Accessory port I/O line (1-3)
// bool: State - true=High, false=Low
int CLuaRoutine::lua_acc_io_write(lua_State *L)
{
    int io_line = lua_tointeger(L, 1);
    bool state = lua_toboolean(L, 2);

    ExtInputPort io_port;
    switch (io_line)
    {
        case 1: io_port = ExtInputPort::ACC_IO_1; break;
        case 2: io_port = ExtInputPort::ACC_IO_2; break;
        case 3: io_port = ExtInputPort::ACC_IO_3; break;
        default:
            return 0;
    }

    acc_port.set_io_port_state(io_port, state);

    return 1;
}

