#include <ArduinoJson.h>
#include "CWsConnection.h"

CWsConnection::CWsConnection(struct tcp_pcb *pcb)
{
    printf("CWsConnection::CWsConnection()\n");
    _pcb = pcb;
}

CWsConnection::~CWsConnection()
{
    printf("~CWsConnection()\n");
    if (_lua_load)
    {
        delete _lua_load;
        _lua_load = NULL;
    }
}

void CWsConnection::callback(uint8_t *data, u16_t data_len, uint8_t mode)
{
    printf("CWsConnection::callback()\n");
    std::string message((char*)data, data_len);

    printf("msg = %s\n", message.c_str());
    
    // Test that the message can be deserialized
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message.c_str());
  
    if (error)
    {
        printf("deserializeJson() failed: %s", error.c_str());
        send_ack("ERROR", -1);
        return;
    }

    // If there's already a message waiting to be processe, give up now (don't alter _json_message)
    if (_pending_message)
    {
        printf("CWsConnection::callback(): Error, already have an unprocessed pending message\n");
        int msgCount = doc["MsgCount"];
        send_ack("ERROR", msgCount);
        return;
    }

    // Message can be deserialized and we don't have a message, so deserialize in into _json_message
    error = deserializeJson(_json_message, message.c_str());
    if (error)
    {
        printf("deserializeJson() failed: %s", error.c_str());
        send_ack("ERROR", -1);
        return;    
    }
    _pending_message = true;
}

void CWsConnection::set_state(state_t new_state)
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
                        std::bind(&CWsConnection::send    , this, std::placeholders::_1),
                        std::bind(&CWsConnection::send_ack, this, std::placeholders::_1, std::placeholders::_2));
            break;

        case state_t::DEAD:
            break;
    }

    if 
    (
        _state    == state_t::LUA_LOAD && 
        new_state != state_t::LUA_LOAD && 
        _lua_load != NULL
    )
    {
        delete _lua_load;
        _lua_load = NULL;
    }

    _state = new_state;
}

void CWsConnection::send_ack(std::string result, int msg_count)
{
    StaticJsonDocument<200> doc;

    doc["Type"] = "Ack";
    doc["MsgCount"] = msg_count;
    doc["Result"] = result;

    std::string generatedJson;
    serializeJson(doc, generatedJson);
    send(generatedJson);
}

void CWsConnection::loop()
{
    if (_state == state_t::DEAD)
        return;

    if (_pcb->state != ESTABLISHED)
    {
        printf("CWsConnection::loop(): connection closed\n");
        _state = state_t::DEAD;
        return;
    }


    if (_pending_message)
    {
        printf("CWsConnection::loop() processing pending message\n");
        std::string msgType = _json_message["Type"];
        if (msgType == "LuaStart")
        {
            set_state(state_t::LUA_LOAD);
        }

        if (_state == state_t::LUA_LOAD)
        {
            if (_lua_load->process(&_json_message))
            {
                // process returns true when lua load is finished
                set_state(state_t::ACTIVE);
            }
        }
        else
        {
            int msgCount = _json_message["MsgCount"];
            send_ack("OK", msgCount);
        }

        _json_message.clear();
        _pending_message = false;
    }

}

void CWsConnection::send(std::string message)
{
    websocket_write(_pcb, (const uint8_t*)message.c_str(), message.length(), 0x01);
}

bool CWsConnection::active()
{
    return (_state != state_t::DEAD);
}
