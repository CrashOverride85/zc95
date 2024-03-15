#ifndef _CRADIO_H
#define _CRADIO_H

#include <inttypes.h>
#include "pico/cyw43_arch.h"

#include "CAnalogueCapture.h"

class CRadio
{
    public:
        CRadio(CAnalogueCapture *analogue_capture);
        ~CRadio();

        bool bluetooth(bool required);
        bool wifi(bool required);
        void loop();

    private:
        CAnalogueCapture *_analogue_capture;

        void set_radio();

        bool _radio_active = false;
        bool _bluetooth = false;
        bool _wifi = false;
};

#endif