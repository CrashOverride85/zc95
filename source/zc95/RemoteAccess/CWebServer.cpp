#include "CWebServer.h"

#include "httpd.h"

CWebServer::CWebServer()
{
    printf("CWebServer()\n");
}

void CWebServer::start()
{
    if (_started)
        return;
    
    printf("CWebServer::start(): starting http server\n");

    httpd_init(0);

}
