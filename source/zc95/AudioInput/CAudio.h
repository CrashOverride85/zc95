#ifndef _CAUDIO_H
#define _CAUDIO_H

#include <inttypes.h>
#include <stdio.h>
#include "AudioTypes.h"

#include "CMCP4651.h"
#include "CAudio3Process.h"
#include "../CControlsPortExp.h"
#include "../CAnalogueCapture.h"
#include "../CSavedSettings.h"
#include "../CUtil.h"
#include "../core1/CRoutineOutput.h"
#include "../display/CHorzBarGraph.h"

#define FFT_N       256  // Must be a power of 2. Also needs to be <= (CAPTURE_DEPTH/3), i.e. the number of audio samples available (/3 for L, R & battery monitoring)
#define SAMPLEFREQ  (SAMPLES_PER_SECOND/6)

class CAudio
{
    public:
        CAudio(CAnalogueCapture *analogueCapture, CMCP4651 *mcp4651, CControlsPortExp *controls);
        ~CAudio();
        void set_audio_digipot_found(bool found);
        void init(CSavedSettings *saved_settings, CDisplay *display);
        void set_routine_output(CRoutineOutput *routine_output);
        audio_hardware_state_t get_audio_hardware_state();

        void get_audio_buffer(CAnalogueCapture::channel chan, uint16_t *samples, uint8_t **buffer);
        void set_gain(CAnalogueCapture::channel chan, uint8_t value); // 0-255, higher=more gain
        void get_current_gain(uint8_t *out_left, uint8_t *out_right);
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);
        void audio_input_enable(bool enable);

        void increment_trigger_point();
        void decrement_trigger_point();

        void do_fft(uint16_t sample_count, uint8_t *buffer);
        void draw_audio_view(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
        void draw_audio_fft_threshold(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

        void audio3(uint16_t sample_count, uint8_t *sample_buffer_left, uint8_t *sample_buffer_right);
        void audio_intensity(uint16_t sample_count, uint8_t *sample_buffer_left, uint8_t *sample_buffer_right);
        void audio_virtual_3(uint16_t sample_count, uint8_t *sample_buffer_left, uint8_t *sample_buffer_right);

        void draw_audio_wave(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool include_gain, bool mono);
        void draw_audio_wave_channel(uint16_t sample_count, uint8_t *sample_buffer, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, color_t colour);
        void draw_audio_virt3(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool include_gain);

        void process();
        void set_audio_mode(audio_mode_t audio_mode);
        bool is_audio_update_available(bool reset);

    private:
        uint16_t get_bar_height(float sample);
        uint16_t get_bar_height(uint8_t *buffer, uint16_t sample_pos, uint8_t sample_count);
        void threshold_cross_process_and_send();
        void get_intensity(uint16_t sample_count, uint8_t *buffer, uint8_t *out_intensity);
        uint8_t *get_virtual_channel(uint16_t sample_count, uint8_t *sample_buffer_left, uint8_t *sample_buffer_right);
        const uint8_t AudioDisplayWidth = 114;

        bool _digipot_found;
        float _fft_output[FFT_N];
        uint16_t _trigger_point = 20;
        uint16_t _max_trigger_point = 20;
        CRoutineOutput *_routine_output;
        float _fundamental_freq;
        uint64_t _last_audio_capture_time_us;
        audio_mode_t _audio_mode = audio_mode_t::OFF;
        bool _audio_update_available;
        CSavedSettings *_saved_settings;
        CAudio3Process *_audio3_process[3]; // Left, right & virtual
        uint8_t _gain_l = 0;
        uint8_t _gain_r = 0;
        CInteruptableSection _interuptable_section;

        CAnalogueCapture *_analogueCapture; // Captures audio using ADC
        CMCP4651 *_mcp4651; // controls digital potentiometer for setting gain
        CControlsPortExp *_controlsPortExp; // Port expander used to (amongst other things) enable/disable microphone power and preamp
        CDisplay *_display;

};

#endif
