#ifndef _CROTENC_H
#define _CROTENC_H

#include <inttypes.h>

class CRotEnc
{
    public:
        CRotEnc();
        void process(uint8_t a, uint8_t b);
        int8_t get_rotary_encoder_change();

    private:
        uint8_t _last_a;
        uint8_t _last_b;
        int8_t _change_since_last_check; // should generally be -1, 0, or 1
};

#endif
