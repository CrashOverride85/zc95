#ifndef _CCDAC_H
#define _CCDAC_H

#include <inttypes.h>

class CDac
{
    public:
        enum class dac_channel
        {
            A,
            B,
            C,
            D
        };

        virtual void set_channel_value(dac_channel channel, uint16_t value) = 0;
};
#endif

