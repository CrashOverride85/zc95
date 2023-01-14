#include <ArduinoJson.h>
#include "CWsConnection.h"

CWsConnection::CWsConnection(struct tcp_pcb *pcb, CAnalogueCapture *analogue_capture, CRoutineOutput *routine_output)
{
    printf("CWsConnection::CWsConnection()\n");
    _pending_message_buffer = (char*)calloc(MAX_WS_MESSAGE_SIZE+1, sizeof(char));
    _pending_message = false;
    _pcb = pcb;
    _analogue_capture = analogue_capture;
    _routine_output = routine_output;
}

CWsConnection::~CWsConnection()
{
    printf("~CWsConnection()\n");
    if (_lua_load)
    {
        delete _lua_load;
        _lua_load = NULL;
    }

    if (_pending_message_buffer)
    {
        free(_pending_message_buffer);
        _pending_message_buffer = NULL;
    }
}

// Process web socket message. This is called from the wifi/tcp polling thread, and is supposed to return quickly.
// So where possible, put incoming messages into _pending_message_buffer for later processing from loop()
void CWsConnection::callback(uint8_t *data, u16_t data_len, uint8_t mode)
{
    printf("CWsConnection::callback()\n");
    std::string message((char*)data, data_len);

    printf("msg = %s\n", message.c_str());
    
    if (data_len > MAX_WS_MESSAGE_SIZE) // Check is important: later mempcy into _pending_message_buffer assumes data_len <= MAX_WS_MESSAGE_SIZE
    {
        printf("CWsConnection::callback(): error: message is too large (%d bytes)\n", data_len);
        send_ack("ERROR", -1);
        return;
    }

    if (_pending_message)
    {
        printf("CWsConnection::callback(): Error, already have an unprocessed pending message\n");

        // Deserialize message just to get MsgCount to include in error response. Might remove this.
        StaticJsonDocument<MAX_WS_MESSAGE_SIZE> doc;
        DeserializationError error = deserializeJson(doc, message.c_str());
        if (error)
        {
            printf("deserializeJson() failed: %s", error.c_str());
            send_ack("ERROR", -1);
            return;
        }

        int msgCount = doc["MsgCount"];
        send_ack("ERROR", msgCount);
        return;
    }
    else
    {
        memcpy(_pending_message_buffer, data, data_len);
        _pending_message_buffer[MAX_WS_MESSAGE_SIZE] = 0;
        _pending_message = true;
    }
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
                        std::bind(&CWsConnection::send_ack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                        _analogue_capture,
                        _routine_output);
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

void CWsConnection::send_ack(std::string result, int msg_count, std::string error)
{
    StaticJsonDocument<MAX_WS_MESSAGE_SIZE> doc;

    doc["Type"] = "Ack";
    doc["MsgCount"] = msg_count;
    doc["Result"] = result;

    if (error.length() > 0)
    {
        doc["Error"] = error;
    }

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

        StaticJsonDocument<MAX_WS_MESSAGE_SIZE> doc;
        DeserializationError error = deserializeJson(doc, _pending_message_buffer);
        if (error)
        {
            printf("deserializeJson() failed: %s", error.c_str());
            send_ack("ERROR", -1);
            return;
        }

        std::string msgType = doc["Type"];
        if (msgType == "LuaStart")
        {
            set_state(state_t::LUA_LOAD);
        }

        if (_state == state_t::LUA_LOAD)
        {
            if (_lua_load->process(&doc))
            {
                // process returns true when lua load is finished
                set_state(state_t::ACTIVE);
            }
        }
        else
        {
            int msgCount = doc["MsgCount"];
            send_ack("OK", msgCount);
        }

        doc.clear();
        memset(_pending_message_buffer, 0, MAX_WS_MESSAGE_SIZE);
        _pending_message = false;
    }

    tcp_output(_pcb);   // Send contents of TCP buffer now. Not required (will get sent 
                        // eventually without), but things are much faster with this.
}

void CWsConnection::send(std::string message)
{
    websocket_write(_pcb, (const uint8_t*)message.c_str(), message.length(), 0x01);
}

bool CWsConnection::active()
{
    return (_state != state_t::DEAD);
}
