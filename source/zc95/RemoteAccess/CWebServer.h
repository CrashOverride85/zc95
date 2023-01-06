#ifndef _CWEBSERVER_H
#define _CWEBSERVER_H

#include <inttypes.h>
#include <stdio.h>
#include <string>

class CWebServer
{
    public:
        CWebServer();
        void start();
        

    private:
        bool _started = false;
};

#endif
