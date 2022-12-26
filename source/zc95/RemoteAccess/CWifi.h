#ifndef _CWIFI_H
#define _CWIFI_H

#include <inttypes.h>
#include <stdio.h>


class CWifi
{
    public:
        CWifi();
        void loop();

    private:
        bool _init_success = false;
};

#endif
