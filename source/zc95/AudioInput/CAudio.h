#ifndef _CAUDIO_H
#define _CAUDIO_H

#include <inttypes.h>
#include <stdio.h>

#include "CMCP4651.h"
#include "../CControlsPortExp.h"
#include "../CAnalogueCapture.h"

class CAudio
{
    public:
        CAudio(CAnalogueCapture *analogueCapture, CMCP4651 *mcp4651, CControlsPortExp *controls);
        void get_audio_buffer(CAnalogueCapture::channel chan, uint16_t *samples, uint8_t **buffer);
        void set_gain(CAnalogueCapture::channel chan, uint8_t value); // 0-255, higher=more gain
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);

    private:
        CAnalogueCapture *_analogueCapture; // Captures audio using ADC
        CMCP4651 *_mcp4651; // controls digital potentiometer for setting gain
        CControlsPortExp *_controlsPortExp; // Port expander used to (amongst other things) enable/disable microphone power and preamp
};

#endif
