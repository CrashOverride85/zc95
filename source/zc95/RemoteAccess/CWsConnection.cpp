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

bool CWsConnection::active()
{
    return (_state == state_t::ACTIVE);
}
