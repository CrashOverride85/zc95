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

#include "../../external/lua/lua-5.1.5/include/lstate.h"

#include "CLuaRoutine.h"
#include "../../../common/zc95_config.h"
#include "../../LuaScripts/LuaScripts.h"
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
    _lua_loop_thread = NULL;
    _lua_loop_thread_ref = LUA_NOREF;
    _suspend_lua_loop_execution_until_us = 0;

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
            { "AccIoSetInput" , &dispatch<&CLuaRoutine::lua_acc_io_input> },
            { "AccSerialWrite", &dispatch<&CLuaRoutine::lua_acc_serial_write> },
            { "EnableTriphase", &dispatch<&CLuaRoutine::lua_enable_triphase> },
            { "LinkChannels"  , &dispatch<&CLuaRoutine::lua_link_channel> },
            { "DelayMs"       , &dispatch<&CLuaRoutine::lua_delay_ms> },
            { "SetMenuOption" , &dispatch<&CLuaRoutine::lua_set_menu_option> },
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

    lua_getglobal(_lua_state, "Config");
    if (lua_istable(_lua_state, -1))
    {
        lua_pushstring(_lua_state, "name");
        lua_gettable(_lua_state, -2);
        conf->name = lua_tostring(_lua_state, -1);
        lua_pop(_lua_state, 1);

        conf->button_text[(int)soft_button::BUTTON_A] = get_string_field("soft_button");
        conf->bluetooth_remote_passthrough = get_bool_field("bluetooth_remote_passthrough");
        conf->force_channel_isolation = !get_bool_field("allow_triphase");
        int loop_freq = get_int_field("loop_freq_hz");

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

        conf->audio_processing_mode = get_audio_processing_mode();

        get_serial_config(&conf->serial);
        get_channel_config(conf->channels);

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

                    case menu_entry_type::AUDIO_VIEW_INTENSITY_STEREO:
                        entry.menu_type = menu_entry_type::AUDIO_VIEW_INTENSITY_STEREO;
                        break;

                    case menu_entry_type::AUDIO_VIEW_INTENSITY_MONO:
                        entry.menu_type = menu_entry_type::AUDIO_VIEW_INTENSITY_MONO;
                        break;

                    case menu_entry_type::AUDIO_VIEW_SPECT:
                    case menu_entry_type::AUDIO_VIEW_WAVE:
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

    // For user uploaded scripts, prefix name with "U:" on pattern list
    if  ((_script == NULL) && (lua_scripts[_script_index].writeable))
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

    if (menu_id >= MENU_ID_CHANNEL5)
    {
        uint8_t channel_id = menu_id - MENU_ID_CHANNEL5 + 4;
        handle_extra_channel_power_change(channel_id, new_value);
        return;
    }

    lua_getglobal(_lua_state, "MinMaxChange");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushinteger(_lua_state, menu_id);
        lua_pushinteger(_lua_state, new_value);
        pcall(2, 0, 0);
    }
    else
    {
        lua_pop(_lua_state, 1);
    }
}

void CLuaRoutine::handle_extra_channel_power_change(uint8_t channel_id, uint8_t power_percent)
{
    if (power_percent > 100)
        return;

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
    else
    {
        lua_pop(_lua_state, 1);
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
        else
        {
            lua_pop(_lua_state, 1);
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
        
        case trigger_socket::Acc:
            str_socket = "ACCESSORY";
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
        
        case trigger_part::C: // only applicable for the accessory port, which has 3 lines
            str_part = "C";
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
    else
    {
        lua_pop(_lua_state, 1);
    }
}

void CLuaRoutine::bluetooth_remote_keypress(CBluetoothRemote::keypress_t key)
{
    std::string keypress = CBluetoothRemote::s_get_keypress_string(key);

    lua_getglobal(_lua_state, "BluetoothRemoteKeypress");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushstring(_lua_state, keypress.c_str());
        pcall(1, 0, 0);
    }
    else
    {
        lua_pop(_lua_state, 1);
    }
}

void CLuaRoutine::audio_intensity(uint8_t left_chan, uint8_t right_chan, uint8_t virt_chan)
{
    lua_getglobal(_lua_state, "AudioIntensityChange");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushinteger(_lua_state, left_chan);
        lua_pushinteger(_lua_state, right_chan);
        lua_pushinteger(_lua_state, virt_chan);
        pcall(3, 0, 0);
    }
    else
    {
        lua_pop(_lua_state, 1);
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

    // purge incoming bluetooth HID event queue to remove any stale entries before starting routine
    CBluetoothConnect::bt_raw_hid_queue_entry_t raw_hid_queue_entry;
    while (queue_try_remove(&gBtRawHidQueue, &raw_hid_queue_entry));

    lua_getglobal(_lua_state, "Setup");
    if (lua_isfunction(_lua_state, -1))
    {
        int32_t time_ms = time_us_64()/1000;
        lua_pushinteger(_lua_state, time_ms);
        pcall(1, 0, 0);
    }
    else
    {
        lua_pop(_lua_state, 1);
        set_all_channels_power(POWER_FULL);
    }

    _get_raw_bt_hid_events = false;
    lua_getglobal(_lua_state, "BluetoothHidEvent");
    if (lua_isfunction(_lua_state, -1))
    {
        printf("Start: have BluetoothHidEvent() function\n");
        _get_raw_bt_hid_events = true;
    }
    lua_pop(_lua_state, 1);

    if (conf.serial.enabled)
        start_acc_serial(&conf.serial);
}

void CLuaRoutine::loop(uint64_t time_us)
{
    channel_pulse_processing();
    load_lua_script_if_required();
    if (!runnable())
        return;

    if (_get_raw_bt_hid_events)
    {
        uint8_t count = 0;
        CBluetoothConnect::bt_raw_hid_queue_entry_t raw_hid_queue_entry;
        while (queue_try_remove(&gBtRawHidQueue, &raw_hid_queue_entry) && count++ < 10)
        {
            lua_getglobal(_lua_state, "BluetoothHidEvent");
            if (lua_isfunction(_lua_state, -1))
            {
                lua_pushinteger(_lua_state, raw_hid_queue_entry.usage_page);
                lua_pushinteger(_lua_state, raw_hid_queue_entry.usage);
                lua_pushinteger(_lua_state, raw_hid_queue_entry.value);
                pcall(3, 0, 0);
            }
        }
    }

    if (_serial_enabled)
        process_serial();

    double time_ms = (double)time_us/(double)1000;

    if (_loop_freq_hz)
    {
        uint32_t loop_freq = floor(time_ms/((double)1000/(double)_loop_freq_hz));
        if (loop_freq == _last_loop)
            return;

        _last_loop = loop_freq;
    }

    run_lua_loop(time_ms);
}

void CLuaRoutine::channel_pulse_processing()
{
    for (uint8_t channel_id = 0; channel_id < CHANNEL_COUNT; channel_id++)
    {
        if (_channel_switch_off_at_us[channel_id])
        {
            if (time_us_64() >  _channel_switch_off_at_us[channel_id])
            {
                channel_off(channel_id);
                _channel_switch_off_at_us[channel_id] = 0;
            }
        }
    }
}

void CLuaRoutine::stop()
{
    if (_lua_state && _lua_loop_thread_ref != LUA_NOREF)
    {
        luaL_unref(_lua_state, LUA_REGISTRYINDEX, _lua_loop_thread_ref);
        _lua_loop_thread_ref = LUA_NOREF;
        _lua_loop_thread = NULL;
    }

    set_all_channels_power(0);
    for (int channel_id=0; channel_id < CHANNEL_COUNT; channel_id++)    
    {
        channel_off(channel_id);
        _channel_switch_off_at_us[channel_id] = 0;
    }

    if (_serial_enabled)
    {
        acc_port.serial_stop();
        _serial_enabled = false;
    }
}

lua_script_state_t CLuaRoutine::lua_script_state()
{
    if (_script_valid == ScriptValid::INVALID)
        return lua_script_state_t::INVALID;
    else
        return lua_script_state_t::VALID;
}

void CLuaRoutine::process_serial()
{
    acc_port.serial_loop();
    std::string serial_data = "";
    if (_serial_mode_line)
    {
        if (acc_port.serial_line_available())
            serial_data = acc_port.serial_get_line();
    }
    else
    {
        while(acc_port.serial_data_available())
        {
            serial_data += acc_port.serial_get_character();

            if (serial_data.length() > 300)
                break;
        }
    }

    if (serial_data.size() == 0)
        return;

    lua_getglobal(_lua_state, "SerialData");
    if (lua_isfunction(_lua_state, -1))
    {
        lua_pushstring(_lua_state, serial_data.c_str());
        pcall(1, 0, 0);
    }
    else
    {
        lua_pop(_lua_state, 1);
    }
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

int CLuaRoutine::run_lua_loop(double time_ms)
{
    if (!_lua_loop_thread)
    {
        if (!create_loop_thread())
        {
            _script_valid = ScriptValid::INVALID;
            stop();
            return -1;
        }
    }

    uint64_t now = time_us_64();

    int nargs = 0;

    if (_suspend_lua_loop_execution_until_us != 0)
    {
        if (now < _suspend_lua_loop_execution_until_us)
            return 0;

        // Resume from inside zc.DelayMs().
        // Don't pass time_ms here, otherwise DelayMs() would return time_ms
        _suspend_lua_loop_execution_until_us = 0;
        nargs = 0;
    }
    else
    {
        // Normal loop: wrapper is waiting for a new time_ms.
        lua_pushnumber(_lua_loop_thread, time_ms);
        nargs = 1;
    }

    _instruction_count = 0;
    int retval = lua_resume(_lua_loop_thread, nargs);

    if (retval == LUA_YIELD)
    {
        return 0;
    }

    if (retval != 0)
    {
        const char *err = lua_tostring(_lua_loop_thread, -1);
        printf("CLuaRoutine::run_lua_loop error: %s retval=%d\n", err, retval);

        print(text_type_t::ERROR, "Script stopped... error: \n%s", err);
        _script_valid = ScriptValid::INVALID;
        stop();
        return retval;
    }

    printf("Lua loop runner unexpectedly finished\n");
    _script_valid = ScriptValid::INVALID;
    stop();
    return -1;
}

bool CLuaRoutine::create_loop_thread()
{
    if (_lua_loop_thread_ref != LUA_NOREF)
    {
        luaL_unref(_lua_state, LUA_REGISTRYINDEX, _lua_loop_thread_ref);
        _lua_loop_thread_ref = LUA_NOREF;
        _lua_loop_thread = NULL;
    }

    _lua_loop_thread = lua_newthread(_lua_state);
    _lua_loop_thread_ref = luaL_ref(_lua_state, LUA_REGISTRYINDEX);

    // Add hook so that an instruction limit can be added
    lua_sethook(_lua_loop_thread, CLuaRoutine::s_lua_hook, LUA_MASKCOUNT, _hook_call_frequency);

    const char *runner =
        "while true do\n"
        "    local time_ms = coroutine.yield()\n"
        "    Loop(time_ms)\n"
        "end\n";

    if (luaL_loadstring(_lua_loop_thread, runner) != 0)
    {
        printf("Failed to load Lua loop runner: %s\n", lua_tostring(_lua_loop_thread, -1));
        luaL_unref(_lua_state, LUA_REGISTRYINDEX, _lua_loop_thread_ref);
        _lua_loop_thread_ref = LUA_NOREF;
        _lua_loop_thread = NULL;
        return false;
    }

    // Start the wrapper. It should run until the first coroutine.yield(),
    // before calling Loop().
    int retval = lua_resume(_lua_loop_thread, 0);

    if (retval != LUA_YIELD)
    {
        printf("Lua loop runner did not yield as expected; retval=%d\n", retval);
        if (retval != 0)
            printf("Error: %s\n", lua_tostring(_lua_loop_thread, -1));

        luaL_unref(_lua_state, LUA_REGISTRYINDEX, _lua_loop_thread_ref);
        _lua_loop_thread_ref = LUA_NOREF;
        _lua_loop_thread = NULL;
        return false;
    }

    return true;
}

void CLuaRoutine::s_lua_hook(lua_State *L, lua_Debug *ar)
{
    CLuaRoutine *ptr = (CLuaRoutine*)(L->l_G->ud);
    if (ptr)
    {
        ptr->lua_hook(L, ar);
    }
}

// If a Lua function continuously executes for more than about LUA_MAX_INSTRUCTIONS, kill the 
// script. This is to protect from infinite loops locking up the box.
void CLuaRoutine::lua_hook(lua_State *L, lua_Debug *ar)
{
    if (ar->event == LUA_HOOKCOUNT)
    {
        _instruction_count += _hook_call_frequency;
        if (_instruction_count > LUA_MAX_INSTRUCTIONS)
        {
            printf("CLuaRoutine(): Terminating execution of Lua script due to instruction limit being reached\n");
            luaL_error(L, "Instruction limit reached");
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
    else if (!strcasecmp(type, "AUDIO_VIEW_INTENSITY_STEREO"))
        *menu_type_out = menu_entry_type::AUDIO_VIEW_INTENSITY_STEREO;
    else if (!strcasecmp(type, "AUDIO_VIEW_INTENSITY_MONO"))
        *menu_type_out = menu_entry_type::AUDIO_VIEW_INTENSITY_MONO;  
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

void CLuaRoutine::get_serial_config(serial_config_t* serial_config)
{
    lua_getfield(_lua_state, -1, "serial");
    if (lua_istable(_lua_state, -1))
    {
        serial_config->enabled   = get_bool_field("enabled", false);
        serial_config->baud      = get_int_field("baud", 115200);
        serial_config->stop_bits = get_int_field("stop_bits", 1);
        serial_config->line_mode = get_bool_field("line_mode", true); 
        _serial_mode_line        = serial_config->line_mode;

        std::string parity       = get_string_field("parity", "NONE");

        if (parity == "ODD")
            serial_config->parity = uart_parity_t::UART_PARITY_ODD;
        else if (parity == "EVEN")
            serial_config->parity = uart_parity_t::UART_PARITY_EVEN;
        else
            serial_config->parity = uart_parity_t::UART_PARITY_NONE;

        if (serial_config->stop_bits != 1 && serial_config->stop_bits != 2)
            serial_config->stop_bits = 1;
    }
    else
    {
        serial_config->enabled = false;
    }
    lua_pop(_lua_state, 1);
}

void CLuaRoutine::get_channel_config(std::vector<channel_config_t> &channels)
{
    constexpr int existing_channel_count = 4;
    constexpr int maximum_channel_count = 10;

    if (channels.size() != existing_channel_count)
    {
        printf("CLuaRoutine::get_channel_config: Expected %d existing channels, but found %u\n", existing_channel_count, channels.size());
        return;
    }

    // The Config table is expected to be at the top of the Lua stack
    lua_getfield(_lua_state, -1, "channels");

    if (lua_isnil(_lua_state, -1))
    {
        lua_pop(_lua_state, 1);
        return;
    }

    if (!lua_istable(_lua_state, -1))
    {
        printf("CLuaRoutine::get_channel_config: Config.channels is not a table\n");
        lua_pop(_lua_state, 1);
        return;
    }

    int highest_channel_number = existing_channel_count;

    /*
     * Validate all keys before modifying the output vector.
     *
     * Valid keys must be integer numbers from 1 to 10. Channels 1-4 are
     * already present in the vector and are therefore left unchanged.
     */
    lua_pushnil(_lua_state);
    while (lua_next(_lua_state, -2) != 0)
    {
        // Key is at -2; value is at -1
        if (!lua_isnumber(_lua_state, -2))
        {
            printf("CLuaRoutine::get_channel_config: Channel key is not numeric\n");

            lua_pop(_lua_state, 2); // Value and key
            lua_pop(_lua_state, 1); // Channels table
            return;
        }

        int channel_number = lua_tonumber(_lua_state, -2);

        if (channel_number < 1 ||
            channel_number > maximum_channel_count)
        {
            printf("CLuaRoutine::get_channel_config: Channel number %d is outside the valid range 1-%d\n",
                channel_number,
                maximum_channel_count);

            lua_pop(_lua_state, 2);
            lua_pop(_lua_state, 1);
            return;
        }

        if (channel_number > highest_channel_number)
            highest_channel_number = channel_number;

        // Remove the value, retaining the key for lua_next()
        lua_pop(_lua_state, 1);
    }

    /*
     * Append channels 5 through the highest configured channel.
     * Missing or invalid entries are represented by NONE/0.
     */
    for (int channel_number = existing_channel_count + 1; channel_number <= highest_channel_number; channel_number++)
    {
        lua_rawgeti(_lua_state, -1, channel_number);

        if (lua_istable(_lua_state, -1))
        {
            std::string channel_type = get_string_field("channel_type", "NONE");

            int index = get_int_field("index", 0);

            if (index < 0 || index > UINT8_MAX)
            {
                printf("CLuaRoutine::get_channel_config: Invalid index %d for channel %d; using NONE/0\n", index, channel_number);

                channel_type = "NONE";
                index = 0;
            }

            channels.push_back(get_channel_config_t(channel_type, index));
        }
        else
        {
            if (!lua_isnil(_lua_state, -1))
            {
                printf("CLuaRoutine::get_channel_config: Configuration for channel %d is not a table; using NONE/0\n", channel_number);
            }

            channels.push_back(get_channel_config_t("NONE", 0));
        }

        lua_pop(_lua_state, 1);
    }

    // Remove the Config.channels table
    lua_pop(_lua_state, 1);
}

channel_config_t CLuaRoutine::get_channel_config_t(std::string channel_type, uint8_t index)
{
   CChannel_types::channel_type type;

    if (!strcasecmp(channel_type.c_str(), "INTERNAL"))
        type = CChannel_types::channel_type::CHANNEL_INTERNAL;

    else if (!strcasecmp(channel_type.c_str(), "COLLAR"))
        type = CChannel_types::channel_type::CHANNEL_COLLAR;
    
    else if (!strcasecmp(channel_type.c_str(), "NONE"))
        type = CChannel_types::channel_type::CHANNEL_NONE;

    else
    {
        type = CChannel_types::channel_type::CHANNEL_NONE;
        index = 0;
        print(text_type_t::ERROR, "Invalid channel type: %s\n", channel_type);
    }

    return {type, index};
}

int CLuaRoutine::get_int_field(const char *field_name, int default_value)
{
    int number;
    lua_getfield(_lua_state, -1, field_name);

    if (lua_type(_lua_state,-1) == LUA_TNIL)
        number = default_value;
    else
        number = lua_tonumber(_lua_state, -1);

    lua_pop(_lua_state, 1);
    return number;
}

std::string CLuaRoutine::get_string_field(const char *field_name, std::string default_value)
{
    std::string str;
    lua_getfield(_lua_state, -1, field_name);

    if (lua_type(_lua_state,-1) == LUA_TNIL)
        str = default_value;
    else
        str = lua_tostring(_lua_state, -1);

    lua_pop(_lua_state, 1);
    return str;
}

bool CLuaRoutine::get_bool_field(const char *field_name, bool default_value)
{
    bool ret = false;
    lua_getfield(_lua_state, -1, field_name);

    if (lua_isboolean(_lua_state, -1))
    {
        ret = lua_toboolean(_lua_state, -1);
    }
    else
    {
        ret = default_value;
    }

    lua_pop(_lua_state, 1);
    return ret;
}

audio_mode_t CLuaRoutine::get_audio_processing_mode()
{
    std::string s = get_string_field("audio_processing_mode");
    if (s == "AUDIO_INTENSITY")
        return audio_mode_t::AUDIO_INTENSITY;
    else
        return audio_mode_t::OFF;
}

bool CLuaRoutine::is_channel_number_valid(int channel_number)
{
    if (channel_number >= 1 && channel_number <= 4)
        return true;
    else
        return false;
}

void CLuaRoutine::start_acc_serial(serial_config_t* serial_config)
{
    if (g_SavedSettings->get_debug_dest() == CSavedSettings::setting_debug::ACC_PORT)
    {
        printf("CLuaRoutine::start_acc_serial: Accessory port is the debug destination, serial will not be available to script\n");
        print(text_type_t::ERROR, "Serial requested but unavailable: change 'Config -> Hardware Config -> Debug destination' from 'Accessory port'");
    }
    else
    {        
        acc_port.serial_set_baud(serial_config->baud);
        acc_port.serial_set_format(serial_config->stop_bits, serial_config->parity);
        acc_port.serial_set_line_mode(serial_config->line_mode);
        acc_port.serial_start();
        _serial_enabled = true;

        printf("Serial enabled with config:\n");
        printf("\tbaud      = %lu\n", serial_config->baud);
        printf("\tstop_bits = %d\n",  serial_config->stop_bits);
        printf("\tline_mode = %d\n",  serial_config->line_mode);
        printf("\tparity    = %d\n\n",serial_config->parity);
    }
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

    channel_on(chan-1);
    _channel_switch_off_at_us[chan-1] = 0;
    return 0;
}

// Takes one param: channel number (1-4)
int CLuaRoutine::lua_channel_off(lua_State *L)
{
    int chan = lua_tointeger(L, 1);
    if (!is_channel_number_valid(chan)) return 0;

    channel_off(chan-1);
    _channel_switch_off_at_us[chan-1] = 0;
    return 0;
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
    channel_on(chan-1);

    return 0;
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

    channel_set_power(chan-1, power);
    return 0;
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

    channel_set_freq(chan-1, freq);
    return 0;
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

    channel_set_pulse_width(chan-1, pos, neg);
    return 0;
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

    if (state)
        acc_port.io_set_port_state(io_port, ExtInputPortState::OUTPUT_HIGH);
    else
        acc_port.io_set_port_state(io_port, ExtInputPortState::OUTPUT_LOW);

    return 0;
}

// Params:
// int : Accessory port I/O line (1-3) to set to input mode
int CLuaRoutine::lua_acc_io_input(lua_State *L)
{
    int io_line = lua_tointeger(L, 1);

    ExtInputPort io_port;
    switch (io_line)
    {
        case 1: io_port = ExtInputPort::ACC_IO_1; break;
        case 2: io_port = ExtInputPort::ACC_IO_2; break;
        case 3: io_port = ExtInputPort::ACC_IO_3; break;
        default:
            return 0;
    }

    acc_port.io_set_port_state(io_port, ExtInputPortState::INPUT);

    return 0;
}

int CLuaRoutine::lua_acc_serial_write(lua_State *L)
{
    size_t len;
    const char* line = lua_tolstring(L, 1, &len);
    if (len == 0)
        return 0;

    if (_serial_mode_line)
    {
        std::string str(line, len);
        acc_port.serial_write_line(str);
    }
    else
    {
        for(size_t n=0; n < len; n++)
            acc_port.serial_write(line[n]);
    }

    return 0;
}

// Params:
// bool: State - true=enabled, false=disabled
int CLuaRoutine::lua_enable_triphase(lua_State *L)
{
    bool state = lua_toboolean(L, 2);
    set_channel_isolation(state);
    return 0;
}

// Params:
// int : Lead channel
// int : Linked channel
// int : offset percent / how much the channels pulses will overlap. 0% offset = fully overlap
int CLuaRoutine::lua_link_channel(lua_State *L)
{
    int lead   = lua_tointeger(L, 1);
    int linked = lua_tointeger(L, 2);
    int offset = lua_tointeger(L, 3);

    if (!is_channel_number_valid(lead)) return 0;
    if (linked < 0 || linked > 255) return 0;
    if (offset < 0 || offset > 100) return 0;

    channel_link_channel(lead-1, linked-1, offset);
    return 0;
}

// Params:
// int : Milliseconds to suspend execution for (0 - 10000, i.e. 0 to 10 seconds)
int CLuaRoutine::lua_delay_ms(lua_State *L)
{
    int delay_ms   = lua_tointeger(L, 1);

    if (delay_ms < 0 || delay_ms > 10000) return 0;

    CLuaRoutine *ptr = (CLuaRoutine*)(L->l_G->ud);
    if (ptr)
        ptr->_suspend_lua_loop_execution_until_us =
            time_us_64() + ((uint64_t)delay_ms * 1000);

    return lua_yield(L, 0);
}

// Update a menu entry from a script.
// Params:
// int : menu id - as set in Config.menu_items.id in script
// int : value   - value for setting. For MIN_MAX types, should be between the configured 
//                 min & max, for MULTI_CHOICE should match one of the choice_id's
int CLuaRoutine::lua_set_menu_option(lua_State *L)
{
    int menu_id = lua_tointeger(L, 1);
    int value   = lua_tointeger(L, 2);
    
    if (menu_id >= MENU_ID_CHANNEL5 || menu_id < 0)
        return 0;

    if (value > 0xFFFF || value < 0)
        return 0;

    set_menu_value(menu_id, value);

    return 0;
}
