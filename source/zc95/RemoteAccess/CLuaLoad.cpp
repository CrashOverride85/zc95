#include "CLuaLoad.h"
#include "CLuaTest.h"

CLuaLoad::CLuaLoad(std::function<void(std::string)> send_function, std::function<void(std::string result, int msg_count)> send_ack)
{
    printf("CLuaLoad::CLuaLoad()\n");
    _send = send_function;
    _send_ack = send_ack;
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

        int index = (*doc)["Index"];
        int flash_size = get_lua_flash_size(index);
        printf("CLuaLoad: Need %d bytes for lua script index %d\n", flash_size, index);

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
            }
            else
            {
                printf("CLuaLoad::process() bad script!\n\t%s\n", lua_error.c_str());
            }




        }

        // TODO
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

int CLuaLoad::get_lua_flash_size(uint8_t index)
{
    int flash_size = 0;

    // These are definied in LuaScripts.S and are empty contiguous blocks of 24k flash that can be used 
    // to put lua scripts in
    extern uint8_t lua_script1_start;  
    extern uint8_t lua_script2_start;
    extern uint8_t lua_script3_start;
    extern uint8_t lua_script4_start;
    extern uint8_t lua_script5_start;
    extern uint8_t lua_script5_end;

    switch (index)
    {
        case 1:
            flash_size = (uint32_t)&lua_script2_start - (uint32_t)&lua_script1_start;
            break;

        case 2:
            flash_size = (uint32_t)&lua_script3_start - (uint32_t)&lua_script2_start;
            break;

        case 3:
            flash_size = (uint32_t)&lua_script4_start - (uint32_t)&lua_script3_start;
            break;

        case 4:
            flash_size = (uint32_t)&lua_script5_start - (uint32_t)&lua_script4_start;
            break;

        case 5:
            flash_size = (uint32_t)&lua_script5_end   - (uint32_t)&lua_script5_start;
            break;

        default:
            printf("CLuaLoad: Passed invalid lua script index: %d\n", index);
            flash_size = 0;
            break;
    }

    return flash_size;
}

