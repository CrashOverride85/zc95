#ifndef _CAUDIO_H
#define _CAUDIO_H

#include <inttypes.h>
#include <stdio.h>

#include "CMCP4651.h"
#include "../CControlsPortExp.h"
#include "../CAnalogueCapture.h"
#include "../CSavedSettings.h"
#include "../core1/CRoutineOutput.h"

#define FFT_N       256  // Must be a power of 2. Also needs to be <= (CAPTURE_DEPTH/3), i.e. the number of audio samples available (/3 for L, R & battery monitoring)
#define SAMPLEFREQ  (SAMPLES_PER_SECOND/6)

class CAudio
{
    public:
        enum class audio_mode_t
        {
            OFF,
            THRESHOLD_CROSS_FFT
        };

        CAudio(CAnalogueCapture *analogueCapture, CMCP4651 *mcp4651, CControlsPortExp *controls);
        void get_audio_buffer(CAnalogueCapture::channel chan, uint16_t *samples, uint8_t **buffer);
        void set_gain(CAnalogueCapture::channel chan, uint8_t value); // 0-255, higher=more gain
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);

        void increment_trigger_point();
        void decrement_trigger_point();

        void init(CSavedSettings *saved_settings);
        void set_routine_output(CRoutineOutput *routine_output);

        void do_fft(uint16_t sample_count, uint8_t *buffer);

        void draw_audio_view(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

        void draw_audio_fft_threshold(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

        void process();
        void set_audio_mode(audio_mode_t audio_mode);

    private:
        uint16_t get_bar_height(float sample);
        uint16_t get_bar_height(uint8_t *buffer, uint16_t sample_pos, uint8_t sample_count);
        void threshold_cross_process_and_send();
        const uint8_t AudioDisplayWidth = 114;

        float _fft_output[FFT_N];
        uint16_t _trigger_point = 20;
        uint16_t _max_trigger_point = 20;
        CRoutineOutput *_routine_output;
        float _fundamental_freq;
        uint64_t _last_audio_capture_time_us;
        audio_mode_t _audio_mode = audio_mode_t::OFF;        

        CAnalogueCapture *_analogueCapture; // Captures audio using ADC
        CMCP4651 *_mcp4651; // controls digital potentiometer for setting gain
        CControlsPortExp *_controlsPortExp; // Port expander used to (amongst other things) enable/disable microphone power and preamp

        
};

#endif
