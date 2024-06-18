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
 * a websocket/serial connection into flash. 
 * Processes messages:
 *  - LuaStart
 *  - LuaLine
 *  - LuaEnd
 *
 * Received scripts are written to flash in LUA_UPLOAD_BUFFER_SIZE (4096) byte 
 * blocks. The first byte of the script in flash is initially set to 0/NULL, so
 * that if either something goes wrong during the upload, or the script is not
 * valid, it won't show up on the patterns menu.
 * If all is good, this is changed to a newline, which as the first byte of a
 * script should have no impact. 
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
    _lua_buffer = (uint8_t*)calloc(LUA_UPLOAD_BUFFER_SIZE, sizeof(uint8_t));
    if (_lua_buffer == NULL)
    {
        panic("CLuaLoad::CLuaLoad(): ERROR - calloc returned NULL. Out of memory.");
    }
}

CLuaLoad::~CLuaLoad()
{
    printf("CLuaLoad::~CLuaLoad()\n");
    if (_lua_buffer)
    {
        free(_lua_buffer);
        _lua_buffer = NULL;
    }

    if (_lua_storage)
    {
        delete _lua_storage;
        _lua_storage = NULL;
    }
}

void CLuaLoad::reset()
{
    printf("CLuaLoad: reset\n");
    _lua_slot_size = 0;
    _lua_slot_section = 0;
    _lua_buffer_postion = 0;

    if (_lua_buffer)
    {
        memset(_lua_buffer, 0, LUA_UPLOAD_BUFFER_SIZE);
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

    if (_lua_buffer == NULL)
        return true;

    if (msgType == "LuaStart")
    {
        reset();
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
            if (flash_size <= 0)
            {
                printf("CLuaLoad: Error - invalid flash size: %d\n", flash_size);
                retval = true;
            }

            _lua_slot_size = flash_size;
            _lua_buffer_postion = 1; // First byte of script slot will be null to start with. If the script uploads ok and doesn't error out, change to ' ' to enable script
        }
    }
    else if (msgType == "LuaLine")
    {
        if (_index >= 0)
        {
            std::string text = (*doc)["Text"];
            text += "\n";
            const char *text_c = text.c_str();

            size_t i = 0;
            while (*(text_c+i))
            {
                if (_lua_buffer_postion >= LUA_UPLOAD_BUFFER_SIZE)
                {
                    // write section to flash
                    bool store_result = _lua_storage->store_script_section(_index, _lua_slot_section, (const char*)_lua_buffer, LUA_UPLOAD_BUFFER_SIZE);
                    _lua_buffer_postion = 0;
                    memset(_lua_buffer, 0, LUA_UPLOAD_BUFFER_SIZE);
                    if (!store_result)
                    {
                        printf("CLuaLoad: error storing script section %d\n", _lua_slot_section);
                        errorMessage = "Error storing script";
                        retval = true;
                        break;
                    }
                    _lua_slot_section++;

                    if (_lua_slot_section >= (_lua_slot_size / LUA_UPLOAD_BUFFER_SIZE))
                    {
                        printf("CLuaLoad: script too large. New _lua_slot_section=%d, _lua_slot_size=%d, buffer size=%d\n", 
                            _lua_slot_section, _lua_slot_size, LUA_UPLOAD_BUFFER_SIZE);
                        errorMessage = "Script too large";
                        retval = true;
                        break;
                    }
                }

                _lua_buffer[_lua_buffer_postion++] = *(text_c+i);
                i++;
            }
        }
        else
        {
            printf("CLuaLoad: LuaLine without valid LuaStart\n");
            retval = true;
        }
    }
    else if (msgType == "LuaEnd")
    {
        if (_index >= 0)
        {
            // Write what's still in lua_buffer to flash
            bool store_result = _lua_storage->store_script_section(_index, _lua_slot_section, (const char*)_lua_buffer, LUA_UPLOAD_BUFFER_SIZE);
            if (!store_result)
            {
                printf("CLuaLoad: error storing script section %d\n", _lua_slot_section);
                _send_ack("ERROR", msgId, "Failed to store script");
                reset();
                return true;
            }

            const char *lua_script = (const char *)(lua_scripts[_index].start);
            lua_script++; // Skip over inital null
            //printf("lua_script:\n");
            //puts(lua_script);

            CLuaRoutine lua = CLuaRoutine(lua_script);
            bool ret = lua.is_script_valid();
            if (ret)
            {
                printf("CLuaLoad::process() script ok\n");

                // Script is good (or at least not horribly broken), now remove that inital null so it will get processed at startup and show on the patterns list
                memcpy(_lua_buffer, (const char *)(lua_scripts[_index].start), LUA_UPLOAD_BUFFER_SIZE);
                _lua_buffer[0] = '\n';
                bool store_result = _lua_storage->store_script_section(_index, 0, (const char*)_lua_buffer, LUA_UPLOAD_BUFFER_SIZE);
                if (!store_result)
                {
                    // I don't think this should be possible
                    printf("CLuaLoad::process(): failed to clear NULL at script of script\n");
                    _send_ack("ERROR", msgId, "Internal error");
                    reset();
                    return true;                    
                }
            }
            else
            {
                std::string lua_error = lua.get_last_lua_error();
                printf("CLuaLoad::process() bad script!\n\t%s\n", lua_error.c_str());
                _send_ack("ERROR", msgId, "Invalid script: " + lua_error);
                reset();
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
    {
        send_ack("ERROR", msgId, errorMessage);
        reset();
    }
    else
        send_ack("OK", msgId);

    return retval;
}

void CLuaLoad::send_ack(std::string result, int msg_count, std::string error)
{
    if (error == "")
        _send_ack(result, msg_count, "");
    else
        _send_ack(result, msg_count, error);
}

