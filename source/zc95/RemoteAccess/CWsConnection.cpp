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
}

void CWsConnection::callback(uint8_t *data, u16_t data_len, uint8_t mode)
{
    printf("CWsConnection::callback()\n");
    std::string message((char*)data, data_len);

    printf("msg = %s\n", message.c_str());

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message.c_str());
  
    if (error)
    {
        printf("deserializeJson() failed: %s", error.c_str());
        send_ack("ERROR", -1);
        return;
    }

    std::string msgType = doc["Type"];
    int msgCount = doc["MsgCount"];

    send_ack("OK", msgCount);
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
}

void CWsConnection::send(std::string message)
{
    websocket_write(_pcb, (const uint8_t*)message.c_str(), message.length(), 0x01);
}

bool CWsConnection::active()
{
    return (_state == state_t::ACTIVE);
}
