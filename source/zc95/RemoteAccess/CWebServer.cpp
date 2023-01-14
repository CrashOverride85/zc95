#include "CWebServer.h"
#include <queue>

std::map<struct tcp_pcb*, CWsConnection*> CWebServer::_s_ws_connections;
CAnalogueCapture *CWebServer::_s_analogue_capture;
CRoutineOutput *CWebServer::_s_routine_output;

CWebServer::CWebServer(CAnalogueCapture *analogue_capture, CRoutineOutput *routine_output)
{
    printf("CWebServer()\n");
    _s_analogue_capture = analogue_capture;
    _s_routine_output = routine_output;
}

CWebServer::~CWebServer()
{
    printf("~CWebServer()\n");

    std::map<struct tcp_pcb*, CWsConnection*>::iterator it;
    for (it = _s_ws_connections.begin(); it != _s_ws_connections.end(); it++)
    {
        if (it->second != NULL)
            delete it->second;
    }
}

void CWebServer::start()
{
    if (_started)
        return;
    
    printf("CWebServer::start(): starting http server\n");


   // tCGI pCGIs[] = {
   //     {"/websockets", (tCGIHandler) websocket_cgi_handler},
   // };

    websocket_register_callbacks((tWsOpenHandler) websocket_open_cb, (tWsHandler) websocket_cb);

    httpd_init(0);
}

void CWebServer::loop()
{
    std::queue<struct tcp_pcb*> connections_to_remove;
    
    std::map<struct tcp_pcb*, CWsConnection*>::iterator it;
    for (it = _s_ws_connections.begin(); it != _s_ws_connections.end(); it++)
    {
        it->second->loop();

        // Clear up any closed connections
        if (!it->second->active())
        {
            connections_to_remove.push(it->first);
        }
    }

    while (!connections_to_remove.empty())
    {
        delete _s_ws_connections[connections_to_remove.front()];
        _s_ws_connections.erase(connections_to_remove.front());
        connections_to_remove.pop();
    }
}

void CWebServer::websocket_cb(struct tcp_pcb *pcb, uint8_t *data, u16_t data_len, uint8_t mode)
{
    if (_s_ws_connections.count(pcb) != 1)
    {
        printf("websocket_cb: connection not found!\n");
        return;
    }

    _s_ws_connections[pcb]->callback(data, data_len, mode);

}

void CWebServer::websocket_open_cb(struct tcp_pcb *pcb, const char *uri)
{
    printf("WS URI: %s\n", uri);

    if (!strcmp(uri, "/stream")) 
    {
        printf("request for streaming\n");
        _s_ws_connections[pcb] = new CWsConnection(pcb, _s_analogue_capture, _s_routine_output);
    }
}

char *CWebServer::websocket_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    return "/websockets.html";
}
