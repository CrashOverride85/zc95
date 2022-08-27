#ifndef _CACCPORT_H
#define _CACCPORT_H

#include "../EExtInputPort.h"

class CAccPort
{
    public:
        CAccPort();
        ~CAccPort();
        
        void reset();
        void set_io_port_state(enum ExtInputPort output, bool high);

    private:

};

#endif
