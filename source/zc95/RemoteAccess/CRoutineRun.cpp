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

/* 
 * Deal with running a routine using input from a web socket connection. Uses the same
 * method as when ran via the menus i.e. sending messages to core1 to start/stop pass 
 * user input to the routine.
 * Can handle message types:
 *  - PatternStart
 *  - PatternMinMaxChange
 *  - PatternMultiChoiceChange
 *  - PatternSoftButton
 *  - SetPower
 *  - PatternStop
 * 
 * and will send (as needed):
 *  - PowerStatus
 *  - LuaScriptOutput
 *  - LuaScriptError
 * 
 * On being deleted, the destructor will stop any running routine.
 */

#include "CRoutineRun.h"

CRoutineRun::CRoutineRun(
        std::function<void(std::string)> send_function, 
        std::function<void(std::string result, int msg_count, std::string error)> send_ack_func,
        CRoutineOutput *routine_output,
        std::vector<CRoutines::Routine> &routines
)  : _routines(routines)
{
    printf("CRoutineRun()\n");
    _send = send_function;
    _send_ack = send_ack_func;
    _routine_output = routine_output;

    _routine_output->set_text_callback_function(std::bind(&CRoutineRun::script_output, this, std::placeholders::_1));
    _routine_output->set_menu_change_callback_function(std::bind(&CRoutineRun::menu_changed_callback, this, std::placeholders::_1));
}

CRoutineRun::~CRoutineRun()
{
    printf("~CRoutineRun()\n");
    _routine_output->set_menu_change_callback_function(NULL);
    _routine_output->stop_routine();
    _routine_output->set_text_callback_function(NULL);

    for (uint8_t channel = 0; channel < _channel_count; channel++)
        _routine_output->set_remote_power(channel, 0);

    if (_front_panel_power != NULL)
    {
        delete _front_panel_power;
        _front_panel_power = NULL;
    }

    if (_output_power != NULL)
    {
        _output_power = NULL;
        delete _output_power;
    }

    if (_max_output_power != NULL)
    {
        _max_output_power = NULL;
        delete _max_output_power;
    }

    _channel_count = 0;
}

bool CRoutineRun::process(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc)
{
    std::string msgType = (*doc)["Type"];
    int msgId = (*doc)["MsgId"];
    bool pattern_start = false;

    if (msgType == "PatternStart")
    {
        int index = (*doc)["Index"];

        if (index < 0 || index >= (int)_routines.size())
        {
            send_ack("ERROR", msgId);
            return true; // finished
        }

        set_pattern_config(index);
        update_channel_count(_pattern_conf.channels.size());
        _routine_output->activate_routine(index);
        pattern_start = true;
        _running = true;
    }

    else if (msgType == "PatternMinMaxChange")
    {
        int menu_id   = (*doc)["MenuId"];
        int new_value = (*doc)["NewValue"];

        _routine_output->menu_min_max_change(menu_id, new_value);
    }

    else if (msgType == "PatternMultiChoiceChange")
    {
        int menu_id   = (*doc)["MenuId"];
        int choice_id = (*doc)["ChoiceId"];

        _routine_output->menu_multi_choice_change(menu_id, choice_id);
    }

    else if (msgType == "PatternSoftButton")
    {
        int pressed   = (*doc)["Pressed"];
        _routine_output->soft_button_pressed(soft_button::BUTTON_A, pressed != 0);
    }

    else if (msgType == "SetPower")
    {
        for (uint8_t channel = 0; channel < _channel_count; channel++)
        {
            char key[8];
            snprintf(key, sizeof(key), "Chan%u", channel + 1);

            if ((*doc)[key].is<uint16_t>())
            {
                uint16_t channel_power = (*doc)[key];
                _routine_output->set_remote_power(channel, channel_power);
            }
        }
    }

    else if (msgType == "PatternStop")
    {
        // Return value of true means CWsConnection knows we're done and will delete this object.
        // Destructor will then call _routine_output->stop_routine();
        send_ack("OK", msgId);
        _running = false;
        return true;
    }

    else
    {
        send_ack("ERROR", msgId, "Unexpected message");
        return false;
    }

    send_ack("OK", msgId);

    if (pattern_start)
        send_power_status_update();

    return false;
}

void CRoutineRun::loop()
{
    bool update_required = false;

    // If the power levels change (either front panel dial adjusted or changed by script), send update message with new values
    if (time_us_64() - _last_power_status_update_us > (250 * 1000)) // at most every 250ms
    {
        for(uint8_t channel = 0; channel < _channel_count; channel++)
        {
            if (_routine_output->get_output_power(channel) != _output_power[channel])
            {
                _output_power[channel] = _routine_output->get_output_power(channel);
                update_required = true;
            }

            if (_routine_output->get_max_output_power(channel) != _max_output_power[channel])
            {
                _max_output_power[channel] = _routine_output->get_max_output_power(channel);
                update_required = true;
            }

            if (_routine_output->get_front_pannel_power(channel) != _front_panel_power[channel])
            {
                _front_panel_power[channel] = _routine_output->get_front_pannel_power(channel);
                update_required = true;
            }
        }

        if (update_required)
        {
            send_power_status_update();
            _last_power_status_update_us = time_us_64();
        }
    }

    // Watch out for script failing, send a message if this happens
    if (_lua_script_state != _routine_output->get_lua_script_state())
    {
        _lua_script_state = _routine_output->get_lua_script_state();

        if (_lua_script_state == lua_script_state_t::INVALID)
        {
            send_lua_script_error_message();
        }
    }
}

void CRoutineRun::send_lua_script_error_message()
{
    StaticJsonDocument<250> status_message;

    status_message["Type"] = "LuaScriptError";
    status_message["MsgId"] = -1;

    std::string generatedJson;
    serializeJson(status_message, generatedJson);
    _send(generatedJson);
}

void CRoutineRun::send_power_status_update()
{
    DynamicJsonDocument status_message(1500);

    status_message["Type"] = "PowerStatus";
    status_message["MsgId"] = -1;

    JsonArray channels = status_message.createNestedArray("Channels");
    for(uint8_t channel = 0; channel < _channel_count; channel++)
    {
        JsonObject obj = channels.createNestedObject();
        obj["Channel"]        = channel+1;
        obj["OutputPower"]    = _output_power[channel];
        obj["MaxOutputPower"] = _max_output_power[channel];
        obj["PowerLimit"]     = channel < INTERNAL_CHANNEL_COUNT ? _front_panel_power[channel] : 1000;
    }

    std::string generatedJson;
    serializeJson(status_message, generatedJson);
    _send(generatedJson);
}

void CRoutineRun::send_ack(std::string result, int msg_count, std::string error)
{
    _send_ack(result, msg_count, error);
}

void CRoutineRun::script_output(pattern_text_output_t output)
{
    StaticJsonDocument<500> script_output;

    script_output["Type"] = "LuaScriptOutput";
    script_output["MsgId"] = -1;
    script_output["Text"] = output.text;
    script_output["Time"] = output.time_generated_us;

    switch (output.text_type)
    {
        case text_type_t::ERROR:
            script_output["TextType"] = "Error";
            break;

        case text_type_t::PRINT:
            script_output["TextType"] = "Print";
            break;

        default:
            script_output["TextType"] = "Unknown";
            break;
    }

    std::string generatedJson;
    serializeJson(script_output, generatedJson);
    _send(generatedJson);
}

void CRoutineRun::set_pattern_config(uint8_t index)
{
    CRoutines::Routine routine = _routines[index];
    CRoutine* routine_ptr = routine.routine_maker(routine.param);
    routine_ptr->get_routine_config(&_pattern_conf);
    delete routine_ptr;
}

void CRoutineRun::menu_changed_callback(menu_change_msg_t msg)
{
    if (!_running)
        return;

    for (uint8_t idx=0; idx < _pattern_conf.menu.size(); idx++)
    {
        if (_pattern_conf.menu[idx].id == msg.menu_id)
        {
            menu_entry* entry = &_pattern_conf.menu[idx];
            switch (entry->menu_type)
            {
                case menu_entry_type::MIN_MAX:
                    if (msg.new_value >= entry->minmax.min && msg.new_value <= entry->minmax.max)
                    {
                        _routine_output->menu_min_max_change(entry->id, msg.new_value);
                        send_menu_change_update(entry->id, msg.new_value);
                    }
                    return;

                case menu_entry_type::MULTI_CHOICE:
                    {
                        // confirm choice id is valid
                        for (uint8_t c=0; c < entry->multichoice.choices.size(); c++)
                        {
                            if (entry->multichoice.choices[c].choice_id == msg.new_value)
                            {
                                _routine_output->menu_multi_choice_change(entry->id, msg.new_value);
                                send_menu_change_update(entry->id, msg.new_value);
                                return;
                            }
                        }
                    }
                    break;
            }
            break;
        }
    }
}

void CRoutineRun::send_menu_change_update(uint8_t menu_id, uint16_t value)
{
    DynamicJsonDocument status_message(500);

    status_message["Type"] = "MenuOptionChanged";
    status_message["MsgId"] = -1;
    status_message["MenuId"] = menu_id;
    status_message["Value"] = value;

    std::string generatedJson;
    serializeJson(status_message, generatedJson);
    _send(generatedJson);
}

void CRoutineRun::update_channel_count(uint8_t channel_count)
{
    if (channel_count == _channel_count)
    {
        printf("CRoutineRun::update_channel_count - already configured for %d channels, nothing to do.\n", channel_count);
        return;
    }

    printf("CRoutineRun::update_channel_count - configuring for %d channels\n", channel_count);
    if (_front_panel_power != NULL)
    {
        delete _front_panel_power;
        _front_panel_power = NULL;
    }

    if (_output_power != NULL)
    {
        delete _output_power;
        _output_power = NULL;
    }

    if (_max_output_power != NULL)
    {
        delete _max_output_power;
        _max_output_power = NULL;
    }

    _channel_count = channel_count;

    if (_channel_count > 0)
    {
        _front_panel_power = new uint16_t[channel_count]();
        _output_power      = new uint16_t[channel_count]();
        _max_output_power  = new uint16_t[channel_count]();

        for (uint8_t channel=0; channel < _channel_count; channel++)
        {
            _output_power[channel]      = _routine_output->get_output_power(channel);
            _max_output_power[channel]  = _routine_output->get_max_output_power(channel);
            _front_panel_power[channel] = _routine_output->get_front_pannel_power(channel);
        }
    }
}
