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

/* Deal with sanity checking and saving Lua scripts being sent over
 * a websocket connection into flash. 
 * Processes messages:
 *  - LuaStart
 *  - LuaLine
 *  - LuaEnd
 */

#include "CLuaLoad.h"
#include "globals.h"
#include "LuaScripts/LuaScripts.h"
#include "../core1/routines/CLuaRoutine.h"

CLuaLoad::CLuaLoad(
        std::function<void(std::string)> send_function, 
        std::function<void(std::string result, int msg_count, std::string error)> send_ack_func
)
{
    printf("CLuaLoad::CLuaLoad()\n");
    _send = send_function;
    _send_ack = send_ack_func;
    _lua_storage = new CLuaStorage();
}

CLuaLoad::~CLuaLoad()
{
    printf("CLuaLoad::~CLuaLoad()\n");
    if (_lua_buffer)
    {
        free(_lua_buffer);
        _lua_buffer = NULL;
        _lua_buffer_size = 0;
        _lua_buffer_postion = 0;
    }

    if (_lua_storage)
    {
        delete _lua_storage;
        _lua_storage = NULL;
    }
}

// Returns true when finished processing Lua messages. Which is either on receiving a LuaEnd
// message, or on an error processing LuaStart/LuaLine/LuaEnd
bool CLuaLoad::process(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc)
{
    std::string msgType = (*doc)["Type"];
    int msgId = (*doc)["MsgId"];
    bool retval = false;
    std::string errorMessage = "";

    if (msgType == "LuaStart")
    {
        if (_lua_buffer)
        {
            free(_lua_buffer);
            _lua_buffer = NULL;
            _lua_buffer_size = 0;
            _lua_buffer_postion = 0;
        }

        _index = (*doc)["Index"];

        if (_index >= lua_script_count())
        {
            printf("CLuaLoad: ERROR: Lua script index %d is not valid\n", _index);
            errorMessage = "Invalid script index";
            retval = true;
        }
        else if (!lua_script_is_writable(_index))
        {
            printf("CLuaLoad: ERROR: Lua script index %d is not writable\n", _index);
            errorMessage = "Not writable";
            retval = true;
        }
        else
        {
            int flash_size = _lua_storage->get_lua_flash_size(_index);
            printf("CLuaLoad: Need %d bytes for lua script index %d\n", flash_size, _index);

            if (flash_size != 0)
            {
                _lua_buffer_size = flash_size;
                _lua_buffer = (uint8_t*)calloc(_lua_buffer_size, sizeof(uint8_t));
                _lua_buffer_postion = 0;
            }

            if (_lua_buffer == NULL)
            {
                printf("CLuaLoad: NULL lua buffer. Invalid index or out of memory?\n");
                retval = true;
            }
        }
    }
    else if (msgType == "LuaLine")
    {
        if (_lua_buffer)
        {
            std::string text = (*doc)["Text"];
            text += "\n";
            if (_lua_buffer_postion + text.length() + 4 >= _lua_buffer_size)
            {
                printf("CLuaLoad: Buffer full! Lua script too large\n");
                errorMessage = "Script too large";
                retval = true;
            }
            else
            {
                memcpy(_lua_buffer + _lua_buffer_postion, text.c_str(), text.length());
                _lua_buffer_postion += text.length();
            }
        } 
        else
        {
            // NULL _lua_buffer
            printf("CLuaLoad: LuaLine without valid LuaStart\n");
            retval = true;
        }
    }
    else if (msgType == "LuaEnd")
    {
        if (_lua_buffer)
        {
           // printf("_lua_buffer_\n");
           // puts((char*)_lua_buffer);

            CLuaRoutine lua = CLuaRoutine((const char*)_lua_buffer);
            bool ret = lua.is_script_valid();
            if (ret)
            {
                printf("CLuaLoad::process() script ok\n");
                _lua_storage->store_script(_index, (const char*)_lua_buffer, _lua_buffer_size);
                _routines_updated = true;
            }
            else
            {
                std::string lua_error = lua.get_last_lua_error();
                printf("CLuaLoad::process() bad script!\n\t%s\n", lua_error.c_str());
                _send_ack("ERROR", msgId, "Invalid script: " + lua_error);
                return true;
            }
        }

        send_ack("OK", msgId);
        return true; // This is the only case where a true return value is not also an error
    }
    else
    {
        printf("CLuaLoad: Unexpected message type: %s\n", msgType.c_str());
        retval = true;
    }

    if (retval)
        send_ack("ERROR", msgId, errorMessage);
    else
        send_ack("OK", msgId);

    return retval;
}

bool CLuaLoad::routines_updated()
{
    return _routines_updated;
}

void CLuaLoad::send_ack(std::string result, int msg_count, std::string error)
{
    if (error == "")
        _send_ack(result, msg_count, "");
    else
        _send_ack(result, msg_count, error);
}

