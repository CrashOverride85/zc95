#ifndef _CMSGDAC_H
#define _CMSGDAC_H

#include "CDac.h"
#include <inttypes.h>


class CMsgDac : public CDac
{
    public:
        CMsgDac();
        void set_channel_value(dac_channel channel, uint16_t value);

//    private:


};

#endif
