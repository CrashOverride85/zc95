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
)
{
    printf("CRoutineRun()\n");
    _send = send_function;
    _send_ack = send_ack_func;
    _routine_output = routine_output;
    _routines = routines;

    _routine_output->set_text_callback_function(std::bind(&CRoutineRun::script_output, this, std::placeholders::_1));

    for (uint8_t channel=0; channel < MAX_CHANNELS; channel++)
    {
        _output_power[channel]     = _routine_output->get_output_power(channel);
        _max_output_power[channel] = _routine_output->get_max_output_power(channel);
        _front_panel_power[channel] = _routine_output->get_front_pannel_power(channel);
    }
}

CRoutineRun::~CRoutineRun()
{
    printf("~CRoutineRun()\n");
    _routine_output->stop_routine();
    _routine_output->set_text_callback_function(NULL);

    for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
        _routine_output->set_remote_power(channel, 0);
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

        _routine_output->activate_routine(index);
        pattern_start = true;
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
        int channel_power[4] = {0};
        channel_power[0] = (*doc)["Chan1"];
        channel_power[1] = (*doc)["Chan2"];
        channel_power[2] = (*doc)["Chan3"];
        channel_power[3] = (*doc)["Chan4"];
        
        for (uint8_t channel = 0; channel < 4; channel++)
            _routine_output->set_remote_power(channel, channel_power[channel]);
    }

    else if (msgType == "PatternStop")
    {
        // Return value of true means CWsConnection knows we're done and will delete this object.
        // Destructor will then call _routine_output->stop_routine();
        send_ack("OK", msgId);
        return true;
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
        for(uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
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
    DynamicJsonDocument status_message(1000);

    status_message["Type"] = "PowerStatus";
    status_message["MsgId"] = -1;

    JsonArray channels = status_message.createNestedArray("Channels");
    for(uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
    {
        JsonObject obj = channels.createNestedObject();
        obj["Channel"]        = channel+1;
        obj["OutputPower"]    = _output_power[channel];
        obj["MaxOutputPower"] = _max_output_power[channel];
        obj["PowerLimit"]     = _front_panel_power[channel];
    }

    std::string generatedJson;
    serializeJson(status_message, generatedJson);
    _send(generatedJson);
}

void CRoutineRun::send_ack(std::string result, int msg_count)
{
    _send_ack(result, msg_count, "");
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
