#ifndef _CWLUALOAD_H
#define _CWLUALOAD_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <functional>
#include <ArduinoJson.h>
#include "../config.h"

class CLuaLoad
{
    public:
        CLuaLoad(
                std::function<void(std::string)> send_function,
                std::function<void(std::string result, int msg_count)> send_ack);
        ~CLuaLoad();
        bool process(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);

    private:
        std::function<void(std::string)> _send;
        std::function<void(std::string result, int msg_count)> _send_ack;

        int get_lua_flash_size(uint8_t index);

        uint8_t *_lua_buffer = NULL;
        size_t _lua_buffer_size = 0;
        uint _lua_buffer_postion = 0;
};

#endif
