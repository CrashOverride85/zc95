#include "CRotEnc.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Process input from a rotary encoder. Pretty sure this can be significantly improved.
 */

CRotEnc::CRotEnc()
{
    _last_a = 0;
    _last_b = 0;
}

void CRotEnc::process(uint8_t a, uint8_t b)
{
    if (a == _last_a && b == _last_b)
        return;

    if (abs(_change_since_last_check) < 100)
    {
        if (a && b)
        {
            if (_last_a && !_last_b)
            {
                _change_since_last_check++;
            }
            else if (!_last_a && _last_b)
            {
                _change_since_last_check--; // ok
            }
        }
    }        

    _last_a = a;
    _last_b = b;
}

int8_t CRotEnc::get_rotary_encoder_change()
{
    int8_t retval = _change_since_last_check;
    _change_since_last_check = 0;
    return retval;
}
