#ifndef _CWLUALOAD_H
#define _CWLUALOAD_H

#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <functional>
#include <ArduinoJson.h>
#include "../CLuaStorage.h"
#include "../config.h"

class CLuaLoad
{
    public:
        CLuaLoad(
            std::function<void(std::string)> send_function, 
            std::function<void(std::string result, int msg_count, std::string error)> send_ack_func);
        ~CLuaLoad();
        bool process(StaticJsonDocument<MAX_WS_MESSAGE_SIZE> *doc);

    private:
        std::function<void(std::string)> _send;
        std::function<void(std::string result, int msg_count, std::string error)> _send_ack;
        void send_ack(std::string result, int msg_count, std::string error = "");
        void reset();

        uint8_t *_lua_buffer = NULL;
        size_t _lua_slot_size = 0;
        uint8_t _lua_slot_section = 0;
        uint _lua_buffer_postion = 0;
        int _index = -1;
        CLuaStorage *_lua_storage;
};

#endif
