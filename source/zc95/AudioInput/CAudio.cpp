#include "../globals.h"
#include "CAudio.h"

#include <stdlib.h>

#include <hagl_hal.h>
#include <hagl.h>

#include "ESP32_fft.h"

CAudio::CAudio(CAnalogueCapture *analogueCapture, CMCP4651 *mcp4651, CControlsPortExp *controls)
{
    _analogueCapture = analogueCapture;
    _mcp4651 = mcp4651;
    _controlsPortExp = controls;
    _routine_output = NULL;
    _last_audio_capture_time_us = 0;
    _fundamental_freq = 0;
    _audio_update_available = false;
    _saved_settings = NULL;
    _digipot_found = false;
}

void CAudio::set_audio_digipot_found(bool found)
{
    _digipot_found = found;
}

void CAudio::get_audio_buffer(CAnalogueCapture::channel chan, uint16_t *samples, uint8_t **buffer)
{
    _analogueCapture->get_audio_buffer(chan, samples, buffer);
}

void CAudio::set_gain(CAnalogueCapture::channel chan, uint8_t value)
{
    if (_digipot_found)
    {
        if (chan == CAnalogueCapture::channel::LEFT)
            _mcp4651->set_val(0, 255-value);
        else
            _mcp4651->set_val(1, 255-value);
    }
}

void CAudio::mic_preamp_enable(bool enable)
{
    _controlsPortExp->mic_preamp_enable(enable);
}

void CAudio::mic_power_enable(bool enable)
{
    _controlsPortExp->mic_power_enable(enable);
}

void CAudio::audio_input_enable(bool enable)
{
    _controlsPortExp->audio_input_enable(enable);
}

void CAudio::init(CSavedSettings *saved_settings)
{
    set_gain(CAnalogueCapture::channel::LEFT,  saved_settings->get_audio_gain_left ());
    set_gain(CAnalogueCapture::channel::RIGHT, saved_settings->get_audio_gain_right());
    mic_power_enable (saved_settings->get_mic_power_enabled ());
    mic_preamp_enable(saved_settings->get_mic_preamp_enabled());
    _saved_settings = saved_settings;
}

void CAudio::set_routine_output(CRoutineOutput *routine_output)
{
    _routine_output = routine_output;
}

// Taking into account the audio setting in the menu (Auto/present no gain/off) and if the digipot on the audio 
// board was found, return state: NOT_PRESENT, PRESENT_NO_GAIN (basically forced on by menu) or PRESENT
CAudio::audio_hardware_state_t CAudio::get_audio_hardware_state()
{
    if (_saved_settings == NULL)
    {
        printf("CAudio::audio_hardware_state_t CAudio::get_audio_hardware_state(): ERROR - NULL _saved_settings\n");
        return CAudio::audio_hardware_state_t::NOT_PRESENT;
    }

    CAudio::audio_hardware_state_t state;
    switch (_saved_settings->get_audio_setting())
    {
        case CSavedSettings::setting_audio::AUTO:
            if (_digipot_found)
            {
                state = CAudio::audio_hardware_state_t::PRESENT;
            }
            else
            {
                state = CAudio::audio_hardware_state_t::NOT_PRESENT;
            }
            break;

        case CSavedSettings::setting_audio::NO_GAIN:
            state = CAudio::audio_hardware_state_t::PRESENT_NO_GAIN;
            break;

        case CSavedSettings::setting_audio::OFF:
        default:
            state = CAudio::audio_hardware_state_t::NOT_PRESENT;
            break;
    }
    
    return state;
}

void CAudio::increment_trigger_point()
{
    _trigger_point++;
    if (_trigger_point > _max_trigger_point)
        _trigger_point = _max_trigger_point;
}

void CAudio::decrement_trigger_point()
{
    if (_trigger_point > 0)
        _trigger_point--;
}

void CAudio::process()
{
    // If there's no new audio since the last call, return without doing anything
    uint64_t buffer_updated_us = _analogueCapture->get_last_buffer_update_time_us();
    if (buffer_updated_us <= _last_audio_capture_time_us)
        return;

    // There is audio available to process!
    _last_audio_capture_time_us = buffer_updated_us;
    uint16_t sample_count;
    uint8_t *sample_buffer;
   _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::LEFT, &sample_count, &sample_buffer);

    if (_audio_mode == audio_mode_t::OFF)
        return;

    if (_audio_mode == audio_mode_t::THRESHOLD_CROSS_FFT)
    {
        do_fft(sample_count, sample_buffer);
        threshold_cross_process_and_send();
    }

    _audio_update_available = true;
}

bool CAudio::is_audio_update_available(bool reset)
{
    bool state = _audio_update_available;
    
    if (reset)
        _audio_update_available = false;

    return state;
}

void CAudio::set_audio_mode(audio_mode_t audio_mode)
{
    if (_audio_mode != audio_mode)
    {
        printf("Audio mode change: %d -> %d\n", (int)_audio_mode, (int)audio_mode);
        _audio_mode = audio_mode;
    }
}

void CAudio::threshold_cross_process_and_send()
{
    uint16_t bar_height = 0;
    uint8_t thres_cross_count=0;
    uint8_t fft_sample = 0;
    for (uint8_t x=0; x <= AudioDisplayWidth/2; x++)
    {
        bar_height = get_bar_height(_fft_output[fft_sample]);
        if (bar_height > _trigger_point)
        {
            thres_cross_count++;
        }

        fft_sample++;
    }

    if (thres_cross_count && _routine_output != NULL)
    {
        if (_routine_output)
        {
            _routine_output->audio_threshold_reached(_fundamental_freq, thres_cross_count);
        }   
    }
}

uint16_t CAudio::get_bar_height(float sample)
{
    float bar_height;
    float scale_factor = (float)3500 / (float)_max_trigger_point;

    bar_height = sample/scale_factor;
    if (bar_height > _max_trigger_point)
        bar_height = _max_trigger_point;
    
    return bar_height;
}

void CAudio::do_fft(uint16_t sample_count, uint8_t *buffer)
{
    // The FFT logic is slow, and as it's essentially just number crunching (no I/O), it's safe to interupt
    gInteruptable = true;
   
    // These are too big to put on the stack, especially with the recursion used by ESP_fft
    //float fft_input[FFT_N];
    //float fft_output[FFT_N];    
    float *fft_input  = (float*)malloc(sizeof(float)*FFT_N);

    for (int k = 0; k < FFT_N; k++)
        fft_input[k] = (float)buffer[k];

    ESP_fft FFT(FFT_N, SAMPLEFREQ, FFT_REAL, FFT_FORWARD, fft_input, _fft_output);

    // Execute transformation
    FFT.removeDC();
    FFT.hammingWindow();
    FFT.execute();

    FFT.complexToMagnitude();
    _fundamental_freq = FFT.majorPeakFreq();

    free(fft_input);
    gInteruptable = false;
}

void CAudio::draw_audio_view(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    gInteruptable = true;

    _max_trigger_point = y1 - y0;
    if (_trigger_point > _max_trigger_point)
        _trigger_point = _max_trigger_point;

    draw_audio_fft_threshold(x0, y0, x1, y1);

    hagl_draw_rectangle(x0, y0, x1, y1, hagl_color(0x00, 0x00, 0xFF));

    color_t colour = hagl_color(0xFF, 0x00, 0x00);
    hagl_draw_line(x0, y1-_trigger_point, x1, y1-_trigger_point, colour);

    gInteruptable = false;
}
      

void CAudio::draw_audio_fft_threshold(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t width  = x1 - x0;
    uint8_t height = y1 - y0;

    color_t colour = hagl_color(0x00, 0xFF, 0x00);

    uint16_t bar_height = 0;
    uint8_t fft_sample = 0;
    for (uint8_t x=x0; x <= x0+width+2; x+=2)
    {
        bar_height = get_bar_height(_fft_output[fft_sample]);
        if (bar_height > _trigger_point)
            colour = hagl_color(0xFF, 0x00, 0x00);
        else
            colour = hagl_color(0x00, 0xFF, 0x00);

        hagl_draw_line(x0+x  , y1, x0+x  , y1 - bar_height, colour);

        fft_sample++;
    }
}
