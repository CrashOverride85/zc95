#include "CLuaLoad.h"


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
    }
}

bool CLuaLoad::process(StaticJsonDocument<200> *doc)
{
    std::string msgType = (*doc)["Type"];
    int msgCount = (*doc)["MsgCount"];

    if (msgType == "LuaStart")
    {
        if (_lua_buffer)
        {
            free(_lua_buffer);
            _lua_buffer = NULL;
        }

        int index = (*doc)["Index"];
        int flash_size = get_lua_flash_size(index);
        printf("CLuaLoad: Need %d bytes for lua script index %d\n", flash_size, index);

        if (flash_size != 0)
        {
            _lua_buffer = (uint8_t*)malloc(flash_size);
        }

        if (_lua_buffer == NULL)
            _send_ack("ERROR", msgCount);
        else
            _send_ack("OK", msgCount);

    }
    else
        _send_ack("OK", msgCount);



    


    if (msgType == "LuaEnd")
        return true;
    else
        return false;
}

int CLuaLoad::get_lua_flash_size(uint8_t index)
{
    int flash_size = 0;

    // These are definied in LuaScripts.S and are empty contiguous blocks of 24k flash that can be used 
    // put lua scripts in
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

