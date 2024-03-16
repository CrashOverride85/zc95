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
 * Handle processing of incoming messages, either from a websocket connection or serial.
 * Messages are expected to be in JSON format. 
 * Can be used to upload & run Lua scripts, run other routines, set power level, etc.
 * 
 * The more complicated flows are passed off to other classes to deal with:
 *  CLuaLoad    - Transferring/storing Lua scripts
 *  CRoutineRun - Running a routine, dealing with input to pass to it, and sending power change updates
 *                (e.g. front panel power dials changed) to connected client
 */

#include <ArduinoJson.h>
#include <list>
#include "CMessageProcessor.h"
#include "../git_version.h"

CMessageProcessor::CMessageProcessor(
    CRoutineOutput *routine_output, 
    std::vector<CRoutines::Routine> *routines,
    std::function<void(std::string)> send_function)
{
    printf("CMessageProcessor::CMessageProcessor()\n");
    _pending_message_buffer = (char*)calloc(MAX_WS_MESSAGE_SIZE+1, sizeof(char));
    _pending_message = false;
    _routine_output = routine_output;
    _routines = routines;
    _send = send_function;
}

CMessageProcessor::~CMessageProcessor()
{
    printf("~CMessageProcessor()\n");
    if (_lua_load)
    {
        delete _lua_load;
        _lua_load = NULL;
    }

    if (_routine_run)
    {
        delete _routine_run;
        _routine_run = NULL;
    }

    if (_pending_message_buffer)
    {
        free(_pending_message_buffer);
        _pending_message_buffer = NULL;
    }
}

// Process incoming message from web socket or serial. As this can be called from the wifi/tcp polling thread, 
// it is supposed to return quickly. So where possible, put incoming messages into _pending_message_buffer for 
// later processing from loop()
void CMessageProcessor::message(uint8_t *data, u16_t data_len)
{
    printf("CMessageProcessor::message()\n");
    std::string message((char*)data, data_len);

    printf("msg < %s\n", message.c_str());
    
    if (data_len > MAX_WS_MESSAGE_SIZE) // Check is important: later mempcy into _pending_message_buffer assumes data_len <= MAX_WS_MESSAGE_SIZE
    {
        printf("CMessageProcessor::message(): error: message is too large (%d bytes)\n", data_len);
        send_ack("ERROR", -1, "Line too long");
        return;
    }

    if (_pending_message)
    {
        printf("CMessageProcessor::message(): Error, already have an unprocessed pending message\n");

        // Deserialize message just to get MsgCount to include in error response. Might remove this.
        StaticJsonDocument<MAX_WS_MESSAGE_SIZE> doc;
        DeserializationError error = deserializeJson(doc, message.c_str());
        if (error)
        {
            printf("deserializeJson() failed: %s\n", error.c_str());
            send_ack("ERROR", -1);
            return;
        }

        int msgId = doc["MsgId"];
        send_ack("ERROR", msgId);
        return;
    }
    else
    {
        memcpy(_pending_message_buffer, data, data_len);
        _pending_message_buffer[MAX_WS_MESSAGE_SIZE] = 0;
        _pending_message = true;
    }
}

void CMessageProcessor::set_state(state_t new_state)
{
    if (_state == new_state)
        return;

    switch (new_state)
    {
        case state_t::ACTIVE:
            break;

        case state_t::LUA_LOAD:
            if (_lua_load == NULL) // this should always be true
                _lua_load = new CLuaLoad(
                        _send,
                        std::bind(&CMessageProcessor::send_ack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            break;

        case state_t::ROUTINE_RUN:
            if (_routine_run == NULL)
            {
                _routine_run = new CRoutineRun(
                        _send,
                        std::bind(&CMessageProcessor::send_ack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                        _routine_output,
                        _routines);
            }
            break;
    }

    // Delete _lua_load if no longer needed
    if 
    (
        _state    == state_t::LUA_LOAD && 
        new_state != state_t::LUA_LOAD && 
        _lua_load != NULL
    )
    {
        // If LuaLoad changed the routines list (e.g new routine loaded), refresh the list
        if (_lua_load->routines_updated())
        {
            reload_routines();
        }

        delete _lua_load;
        _lua_load = NULL;
    }

    // Delete _routine_run if no longer needed
    if 
    (
        _state    == state_t::ROUTINE_RUN && 
        new_state != state_t::ROUTINE_RUN && 
        _routine_run != NULL
    )
    {
        delete _routine_run;
        _routine_run = NULL;
    }

    _state = new_state;
}

void CMessageProcessor::send_ack(std::string result, int msg_count, std::string error)
{
    StaticJsonDocument<MAX_WS_MESSAGE_SIZE> doc;

    doc["Type"] = "Ack";
    doc["MsgId"] = msg_count;
    doc["Result"] = result;

    if (error.length() > 0)
    {
        doc["Error"] = error;
    }

    std::string generatedJson;
    serializeJson(doc, generatedJson);
    _send(generatedJson);
}

void CMessageProcessor::loop()
{
    if (_pending_message)
    {
        printf("CMessageProcessor::loop() processing pending message\n");

        StaticJsonDocument<MAX_WS_MESSAGE_SIZE> doc;
        DeserializationError error = deserializeJson(doc, _pending_message_buffer);
        if (error)
        {
            printf("deserializeJson() failed: %s", error.c_str());
            send_ack("ERROR", -1);
        
            memset(_pending_message_buffer, 0, MAX_WS_MESSAGE_SIZE);
            _pending_message = false;
            return;
        }

        std::string msgType = doc["Type"];
        if (msgType == "LuaStart")
        {
            set_state(state_t::LUA_LOAD);
        }
        else if (msgType == "PatternStart")
        {
            set_state(state_t::ROUTINE_RUN);
        }

        if (_state == state_t::LUA_LOAD)
        {
            if (_lua_load->process(&doc))
            {
                // process returns true when lua load is finished
                set_state(state_t::ACTIVE);
            }
        }
        else if (_state == state_t::ROUTINE_RUN)
        {
            if (_routine_run->process(&doc))
            {
                // process returns true when routine run is finished
                set_state(state_t::ACTIVE);
            }  
        }
        else if (msgType == "GetLuaScripts")
        {
            send_lua_scripts(&doc);
        }
        else if (msgType == "DeleteLuaScript")
        {
            delete_lua_script(&doc);
        }
        else if (msgType == "GetPatterns")
        {
            send_pattern_list(&doc);
        }
        else if (msgType == "GetPatternDetail")
        {
            send_pattern_detail(&doc);
        }
        else if (msgType == "GetVersion")
        {
            send_version_details(&doc);
        }
        else
        {
            printf("Unknown or unexpected message type: %s\n", msgType.c_str());
            int msgId = doc["MsgId"];
            send_ack("ERROR", msgId);
        }

        doc.clear();
        memset(_pending_message_buffer, 0, MAX_WS_MESSAGE_SIZE);
        _pending_message = false;
    }

    if (_routine_run)
    {
        _routine_run->loop();
    }
}

// Send list of Lua script names loaded in flash (or "<empty>" if nothing loaded in slot)
void CMessageProcessor::send_lua_scripts(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc)
{
    int msg_count = (*doc)["MsgId"];
    StaticJsonDocument<1000> response_message;

    response_message["Type"] = "LuaScripts";
    response_message["MsgId"] = msg_count;
    
    JsonArray scripts = response_message.createNestedArray("Scripts");

    std::list<CLuaStorage::lua_script_t> lua_scripts = CLuaStorage::get_lua_scripts();
    for (std::list<CLuaStorage::lua_script_t>::iterator it = lua_scripts.begin(); it != lua_scripts.end(); ++it)
    {
        JsonObject obj = scripts.createNestedObject();
        obj["Index"] = it->index;
        obj["Empty"] = it->empty;
        obj["Valid"] = it->valid;
        obj["Name"] = it->name;
    }

    response_message["Result"] = "OK";

    std::string generatedJson;
    serializeJson(response_message, generatedJson);
    _send(generatedJson);
}

void CMessageProcessor::delete_lua_script(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc)
{
    int msg_count = (*doc)["MsgId"];
    int index = (*doc)["Index"];
    CLuaStorage lua_storage = CLuaStorage();
    
    if (lua_storage.delete_script_at_index(index))
        send_ack("OK", msg_count);
    else
        send_ack("ERROR", msg_count);

    reload_routines();
}

void CMessageProcessor::send_pattern_list(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc)
{
    int msg_count = (*doc)["MsgId"];
    StaticJsonDocument<2000> response_message;

    response_message["Type"] = "PatternList";
    response_message["MsgId"] = msg_count;
    
    JsonArray patterns = response_message.createNestedArray("Patterns");

    int index=0;
    for (std::vector<CRoutines::Routine>::iterator it = _routines->begin(); it != _routines->end(); it++)
    {
        struct routine_conf conf;
        CRoutine* routine = (*it).routine_maker((*it).param);
        routine->get_config(&conf);

        // Audio stuff probably isn't going to work correctly remotely, so skip
        if (conf.audio_processing_mode == audio_mode_t::OFF)
        {
            JsonObject obj = patterns.createNestedObject();
            obj["Id"] = index;
            obj["Name"] = conf.name;
        }

        index++;
        delete routine;
    }

    response_message["Result"] = "OK";

    std::string generatedJson;
    serializeJson(response_message, generatedJson);
    _send(generatedJson);
}

void CMessageProcessor::send_pattern_detail(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc)
{
    int msg_count = (*doc)["MsgId"];
    int id = (*doc)["Id"];

    StaticJsonDocument<2500> response_message;
    response_message["Type"] = "PatternDetail";
    response_message["MsgId"] = msg_count;

    if (id < 0 || id >= (int)((*_routines).size()))
    {
        printf("CMessageProcessor::send_pattern_detail: invalid id: %d\n", id);
        response_message["Result"] = "ERROR";
    }
    else
    {
        // Get pattern config
        struct routine_conf conf;
        CRoutines::Routine routine = (*_routines)[id];
        CRoutine* routine_ptr = routine.routine_maker(routine.param);
        routine_ptr->get_config(&conf);
        delete routine_ptr;

        response_message["Name"] = conf.name;
        response_message["Id"] = id;
        response_message["ButtonA"] = conf.button_text[(int)soft_button::BUTTON_A];

        JsonArray menu_items = response_message.createNestedArray("MenuItems");
        for (std::vector<menu_entry>::iterator it = conf.menu.begin(); it != conf.menu.end(); it++)
        {
            JsonObject menu_item = menu_items.createNestedObject();
            menu_item["Id"] = it->id;
            menu_item["Title"] = it->title;
            menu_item["Group"] = it->group_id;

            switch (it->menu_type)
            {
                case menu_entry_type::MIN_MAX:
                    menu_item["Type"] = "MIN_MAX";
                    menu_item["Min"] = it->minmax.min;
                    menu_item["Max"] = it->minmax.max;
                    menu_item["IncrementStep"] = it->minmax.increment_step;
                    menu_item["UoM"] = it->minmax.UoM;
                    menu_item["Default"] = it->minmax.current_value;
                    break;

                case menu_entry_type::MULTI_CHOICE:
                {
                    menu_item["Type"] = "MULTI_CHOICE";
                    menu_item["Default"] = it->multichoice.current_selection;

                    JsonArray choices = menu_item.createNestedArray("Choices");
                    for (std::vector<multi_choice_option>::iterator it2 = it->multichoice.choices.begin(); it2 != it->multichoice.choices.end(); it2++)
                    {
                        JsonObject choice = choices.createNestedObject();
                        choice["Id"] = it2->choice_id;
                        choice["Name"] = it2->choice_name;
                    }
                    break;
                }

                case menu_entry_type::AUDIO_VIEW_SPECT:
                case menu_entry_type::AUDIO_VIEW_WAVE:
                case menu_entry_type::AUDIO_VIEW_INTENSITY_STEREO:
                case menu_entry_type::AUDIO_VIEW_INTENSITY_MONO:
                case menu_entry_type::AUDIO_VIEW_VIRTUAL_3:
                    // Not supported for remote access. Would need to stream display updates to the connected GUI,
                    // and the current implementation almost certainly isn't close to being fast enough.
                    // Note that patterns using these menu types are filtered out by send_pattern_list, so the 
                    // connected client shouldn't know the id of the patterns that won't work (although being 
                    // sequential, they're rather easy to guess)
                    break;
                
            }
        }

        response_message["Result"] = "OK";
    }
        
    std::string generatedJson;
    serializeJson(response_message, generatedJson);
    _send(generatedJson);
}

void CMessageProcessor::send_version_details(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc)
{
    int msg_count = (*doc)["MsgId"];
    StaticJsonDocument<250> response_message;

    response_message["Type"] = "VersionDetails";
    response_message["MsgId"] = msg_count;
    response_message["ZC95"] = kGitHash;
    response_message["WsMajor"] = WEBSOCKET_API_VERION_MAJOR;
    response_message["WsMinor"] = WEBSOCKET_API_VERION_MINOR;
    response_message["Result"] = "OK";

    std::string generatedJson;
    serializeJson(response_message, generatedJson);
    _send(generatedJson);
}


void CMessageProcessor::reload_routines()
{
    printf("CMessageProcessor::reload_routines(): routines list has changed, updating\n");
    _routines->clear();
    CRoutines::get_routines(_routines);
}
