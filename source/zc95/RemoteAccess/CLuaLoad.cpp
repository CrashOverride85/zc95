#include "CLuaLoad.h"
#include "CLuaTest.h"

CLuaLoad::CLuaLoad(
        std::function<void(std::string)> send_function, 
        std::function<void(std::string result, int msg_count)> send_ack,
        CAnalogueCapture *analogue_capture, 
        CRoutineOutput *routine_output
)
{
    printf("CLuaLoad::CLuaLoad()\n");
    _send = send_function;
    _send_ack = send_ack;
    _lua_storage = new CLuaStorage(analogue_capture, routine_output);
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
// message, or on an error processing LuaStart or LuaLine
bool CLuaLoad::process(StaticJsonDocument<200> *doc)
{
    std::string msgType = (*doc)["Type"];
    int msgCount = (*doc)["MsgCount"];
    bool retval = false;

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
    else if (msgType == "LuaLine")
    {
        if (_lua_buffer)
        {
            std::string text = (*doc)["Text"];
            text += "\n";
            if (_lua_buffer_postion + text.length() + 4 >= _lua_buffer_size)
            {
                printf("CLuaLoad: Buffer full! Lua script too large\n");
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
            printf("_lua_buffer_\n");
            puts((char*)_lua_buffer);

            CLuaTest lua_test = CLuaTest();
            std::string lua_error;
            bool ret = lua_test.check_script((const char*)_lua_buffer, lua_error);
            if (ret)
            {
                printf("CLuaLoad::process() script ok\n");
                _lua_storage->store_script(_index, (const char*)_lua_buffer, _lua_buffer_size);
            }
            else
            {
                printf("CLuaLoad::process() bad script!\n\t%s\n", lua_error.c_str());
            }




        }

        _send_ack("OK", msgCount);
        return true; // This is the only case where a true return value is not also an error
    }
    else
    {
        printf("CLuaLoad: Unexpected message type: %s\n", msgType.c_str());
        retval = true;
    }

    if (retval)
        _send_ack("ERROR", msgCount);
    else
        _send_ack("OK", msgCount);

    return retval;
}


