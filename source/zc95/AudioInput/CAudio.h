#ifndef _CAUDIO_H
#define _CAUDIO_H

#include <inttypes.h>
#include <stdio.h>

#include "CMCP4651.h"
#include "../CControlsPortExp.h"
#include "../CAnalogueCapture.h"
#include "../CSavedSettings.h"
#include "../core1/CRoutineOutput.h"

class CAudio
{
    public:
        CAudio(CAnalogueCapture *analogueCapture, CMCP4651 *mcp4651, CControlsPortExp *controls);
        void get_audio_buffer(CAnalogueCapture::channel chan, uint16_t *samples, uint8_t **buffer);
        void set_gain(CAnalogueCapture::channel chan, uint8_t value); // 0-255, higher=more gain
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);

        void increment_trigger_point();
        void decrement_trigger_point();

        void init(CSavedSettings *saved_settings);
        void set_routine_output(CRoutineOutput *routine_output);

        void draw_mic_audio_wave(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);


        void draw_mic_audio_wave_v1(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
        void draw_mic_audio_wave_v2(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
        void draw_mic_audio_wave_v3(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

    private:
        uint16_t _trigger_point = 20;
        uint16_t _max_trigger_point = 20;
        CRoutineOutput *_routine_output;
        uint16_t get_bar_height(uint8_t *buffer, uint16_t sample_pos, uint8_t sample_count);

        CAnalogueCapture *_analogueCapture; // Captures audio using ADC
        CMCP4651 *_mcp4651; // controls digital potentiometer for setting gain
        CControlsPortExp *_controlsPortExp; // Port expander used to (amongst other things) enable/disable microphone power and preamp
};

#endif
