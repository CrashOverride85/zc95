/*
 * ZC95
 * Copyright (C) 2022  CrashOverride85
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "../globals.h"
#include "CAudio.h"

#include <stdlib.h>
#include <inttypes.h>

#include <hagl_hal.h>
#include <hagl.h>

#include "ESP32_fft.h"

CAudio::CAudio(CAnalogueCapture *analogueCapture, CMCP4651 *mcp4651, CMainBoardPortExp *controls)
{
    printf("CAudio()\n");
    _analogueCapture = analogueCapture;
    _mcp4651 = mcp4651;
    _controlsPortExp = controls;
    _routine_output = NULL;
    _last_audio_capture_time_us = 0;
    _fundamental_freq = 0;
    _audio_update_available = false;
    _saved_settings = NULL;
    _digipot_found = false;

    _audio3_process[AUDIO_LEFT ] = new CAudio3Process(0, analogueCapture);
    _audio3_process[AUDIO_RIGHT] = new CAudio3Process(1, analogueCapture);
    _audio3_process[AUDIO_VIRT ] = new CAudio3Process(2, analogueCapture);
}

CAudio::~CAudio()
{
    printf("~CAudio()\n");
    for (uint8_t channel = 0; channel <= 1; channel++)
    {
        if (_audio3_process[channel])
        {
            delete _audio3_process[channel];
            _audio3_process[channel] = NULL;
        }
    }
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
        {
            _mcp4651->set_val(0, 255-value);
            _gain_l = value;
        }
        else
        {
            _mcp4651->set_val(1, 255-value);
            _gain_r = value;
        }
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

void CAudio::init(CSavedSettings *saved_settings, CDisplay *display)
{
    _display = display;
    set_gain(CAnalogueCapture::channel::LEFT,  saved_settings->get_audio_gain_left ());
    set_gain(CAnalogueCapture::channel::RIGHT, saved_settings->get_audio_gain_right());
    mic_power_enable (saved_settings->get_mic_power_enabled ());
    mic_preamp_enable(saved_settings->get_mic_preamp_enabled());
    _saved_settings = saved_settings;
    _gain_l = _saved_settings->get_audio_gain_left();
    _gain_r = _saved_settings->get_audio_gain_right();
}

void CAudio::set_routine_output(CRoutineOutput *routine_output)
{
    _routine_output = routine_output;
}

// Taking into account the audio setting in the menu (Auto/present no gain/off) and if the digipot on the audio 
// board was found, return state: NOT_PRESENT, PRESENT_NO_GAIN (basically forced on by menu) or PRESENT
audio_hardware_state_t CAudio::get_audio_hardware_state()
{
    if (_saved_settings == NULL)
    {
        printf("CAudio::audio_hardware_state_t CAudio::get_audio_hardware_state(): ERROR - NULL _saved_settings\n");
        return audio_hardware_state_t::NOT_PRESENT;
    }

    audio_hardware_state_t state;
    switch (_saved_settings->get_audio_setting())
    {
        case CSavedSettings::setting_audio::AUTO:
            if (_digipot_found)
            {
                state = audio_hardware_state_t::PRESENT;
            }
            else
            {
                state = audio_hardware_state_t::NOT_PRESENT;
            }
            break;

        case CSavedSettings::setting_audio::NO_GAIN:
            state = audio_hardware_state_t::PRESENT_NO_GAIN;
            break;

        case CSavedSettings::setting_audio::OFF:
        default:
            state = audio_hardware_state_t::NOT_PRESENT;
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

void CAudio::get_current_gain(uint8_t *out_left, uint8_t *out_right)
{
    *out_left  = _gain_l;
    *out_right = _gain_r;
}

void CAudio::process()
{
    // If there's no new audio since the last call, return without doing anything
    uint64_t buffer_updated_us = _analogueCapture->get_last_buffer_update_time_us();
    if (buffer_updated_us <= _last_audio_capture_time_us)
        return;

    // There is audio available to process!
    _last_audio_capture_time_us = buffer_updated_us;
    uint16_t sample_count_l;
    uint16_t sample_count_r;
    uint16_t sample_count;
    uint8_t *sample_buffer_left;
    uint8_t *sample_buffer_right;
   _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::LEFT,  &sample_count_l, &sample_buffer_left );
   _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::RIGHT, &sample_count_r, &sample_buffer_right);

    if (sample_count_l < sample_count_r)
        sample_count = sample_count_l;
    else
        sample_count = sample_count_r;

    if (_audio_mode == audio_mode_t::OFF)
        return;

    else if (_audio_mode == audio_mode_t::THRESHOLD_CROSS_FFT)
    {
        do_fft(sample_count, sample_buffer_left);
        threshold_cross_process_and_send();
    }

    else if (_audio_mode == audio_mode_t::AUDIO3)
    {
        audio3(sample_count, sample_buffer_left, sample_buffer_right);
    }

    else if (_audio_mode == audio_mode_t::AUDIO_INTENSITY)
    {
        audio_intensity(sample_count, sample_buffer_left, sample_buffer_right);
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
    // The FFT logic is slow, and as it's essentially just number crunching (no I/O), it's safe to interrupt
    _interuptable_section.start();
   
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
    _interuptable_section.end();
}

void CAudio::draw_audio_view(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    _interuptable_section.start();

    _max_trigger_point = y1 - y0;
    if (_trigger_point > _max_trigger_point)
        _trigger_point = _max_trigger_point;

    draw_audio_fft_threshold(x0, y0, x1, y1);

    hagl_draw_rectangle(_display->get_hagl_backed(), x0, y0, x1, y1, hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF));

    hagl_color_t colour = hagl_color(_display->get_hagl_backed(), 0xFF, 0x00, 0x00);
    hagl_draw_line(_display->get_hagl_backed(), x0, y1-_trigger_point, x1, y1-_trigger_point, colour);

    _interuptable_section.end();
}

void CAudio::draw_audio_wave(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool include_gain, bool mono)
{
    uint16_t sample_count;
    uint8_t *sample_buffer_left;
    uint8_t *sample_buffer_right;
    uint8_t bar_height = 2;
    CHorzBarGraph bar_graph = CHorzBarGraph(_display);
    struct display_area bar_graph_area = 
    {
        .x0 = x0,
        .y0 = (int16_t)(y1 - bar_height),
        .x1 = x1,
        .y1 = y1
    };

    _interuptable_section.start();

    if (mono)
    {
        _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::LEFT,  &sample_count, &sample_buffer_left);
        draw_audio_wave_channel(sample_count, sample_buffer_left, x0, y0+2, x1, y1, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    }
    else
    {
        _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::LEFT,  &sample_count, &sample_buffer_left );
        _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::RIGHT, &sample_count, &sample_buffer_right);

        draw_audio_wave_channel(sample_count, sample_buffer_left , x0              , y0+2, x0+((x1-x0)/2)-1, y1, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        draw_audio_wave_channel(sample_count, sample_buffer_right, x0+((x1-x0)/2)+1, y0+2, x1              , y1, hagl_color(_display->get_hagl_backed(), 0xFF, 0x00, 0x00));
    }

    hagl_draw_rectangle(_display->get_hagl_backed(), x0, y0, x1, y1, hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF));

    // Draw gain setting if digipot found
    if (get_audio_hardware_state() == audio_hardware_state_t::PRESENT)
    {
        hagl_color_t yellow  = hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0x00);
        bar_graph.draw_horz_bar_graph(bar_graph_area, 0, 255, _gain_l, "", yellow, true);
    }

    _interuptable_section.end();
}

// Draw an audio waveform from the supplied samples in the designated position. 
// Waveform drawing starts when it crosses zero, which should give a stable waveform display for a periodic signal
void CAudio::draw_audio_wave_channel(uint16_t sample_count, uint8_t *sample_buffer, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, hagl_color_t colour)
{
    // Scale factor to scale 0-255 samples to whatever screen space is available
    float scale_factor = (float)255 / (float)((float)y0-(float)y1);

    // Get average value, so signal can be changed from 0-255 to (about) +/- 128. In theory 127 could be 
    // used, but hardware realities mean it's likely not going to be exactly 127. Basically DC bias removal.
    uint32_t total = 0;
    for (uint16_t x = 0; x < sample_count; x++)
    {
        total += sample_buffer[x];
    }
    uint8_t avg = total / sample_count;

    int16_t prev_value = avg-sample_buffer[0];
    int start_sample_idx;
    for (start_sample_idx = 0; start_sample_idx < sample_count - (x1-x0); start_sample_idx++)
    {
        int16_t current_val = avg-sample_buffer[start_sample_idx];
        // trigger - look for zero cross
        if (prev_value < 0 && current_val >= 0)
        {
            break;
        }
        
        prev_value = current_val;
    }
    
    // Finally draw the waveform, starting at the zero cross point
    uint16_t sample_idx = 0;
    for (int x = x0; x+1 < x1; x++)
    {
        float line_y0 = (float)y1+((float)sample_buffer[start_sample_idx+sample_idx]  / scale_factor);
        float line_y1 = (float)y1+((float)sample_buffer[start_sample_idx+sample_idx+1]/ scale_factor);

        hagl_draw_line(_display->get_hagl_backed(), x, line_y0, x+1, line_y1, colour);
        sample_idx++;
    }
}

void CAudio::draw_audio_virt3(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool include_gain)
{
    uint16_t sample_count;
    uint8_t *sample_buffer_left;
    uint8_t *sample_buffer_right;
    uint8_t *sample_buffer_virt;
    uint8_t bar_height = 2;
    CHorzBarGraph bar_graph = CHorzBarGraph(_display);
    struct display_area bar_graph_area = 
    {
        .x0 = x0,
        .y0 = (int16_t)(y1 - bar_height),
        .x1 = x1,
        .y1 = y1
    };

    _interuptable_section.start();

    _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::LEFT,  &sample_count, &sample_buffer_left );
    _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::RIGHT, &sample_count, &sample_buffer_right);
    sample_buffer_virt = get_virtual_channel(sample_count, sample_buffer_left, sample_buffer_right);
    if (!sample_buffer_virt)
        return;

    uint16_t display_window_width = x1 - x0;
    uint16_t wave_width = display_window_width / 3;

    draw_audio_wave_channel(sample_count, sample_buffer_left , x0+(wave_width * 0), y0+2, x0+(wave_width * 1), y1, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
    draw_audio_wave_channel(sample_count, sample_buffer_right, x0+(wave_width * 1), y0+2, x0+(wave_width * 2), y1, hagl_color(_display->get_hagl_backed(), 0xFF, 0x00, 0x00));
    draw_audio_wave_channel(sample_count, sample_buffer_virt , x0+(wave_width * 2), y0+2, x0+(wave_width * 3), y1, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0x00));

    hagl_draw_rectangle(_display->get_hagl_backed(), x0, y0, x1, y1, hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF));

    // Draw gain setting if digipot found
    if (get_audio_hardware_state() == audio_hardware_state_t::PRESENT)
    {
        hagl_color_t yellow  = hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0x00);
        bar_graph.draw_horz_bar_graph(bar_graph_area, 0, 255, _gain_l, "", yellow, true);
    }

    free (sample_buffer_virt);
}

void CAudio::draw_audio_fft_threshold(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t width  = x1 - x0;

    hagl_color_t colour = hagl_color(_display->get_hagl_backed(), 0x00, 0xFF, 0x00);

    uint16_t bar_height = 0;
    uint8_t fft_sample = 0;
    for (uint8_t x=x0; x <= x0+width+2; x+=2)
    {
        bar_height = get_bar_height(_fft_output[fft_sample]);
        if (bar_height > _trigger_point)
            colour = hagl_color(_display->get_hagl_backed(), 0xFF, 0x00, 0x00);
        else
            colour = hagl_color(_display->get_hagl_backed(), 0x00, 0xFF, 0x00);

        hagl_draw_line(_display->get_hagl_backed(), x0+x  , y1, x0+x  , y1 - bar_height, colour);

        fft_sample++;
    }
}

void CAudio::audio3(uint16_t sample_count, uint8_t *sample_buffer_left, uint8_t *sample_buffer_right)
{
    static uint8_t last_intensity_left  = 0;
    static uint8_t last_intensity_right = 0;
    static uint8_t last_intensity_virt  = 0;
    uint8_t intensity_left  = 0;
    uint8_t intensity_right = 0;
    uint8_t intensity_virt  = 0;

    _interuptable_section.start();

    // generate 3rd channel
    uint8_t *sample_buffer_virt = get_virtual_channel(sample_count, sample_buffer_left, sample_buffer_right);
    if (!sample_buffer_virt)
        return;

    _audio3_process[AUDIO_LEFT ]->process_samples(sample_count, sample_buffer_left , &intensity_left );
    _audio3_process[AUDIO_RIGHT]->process_samples(sample_count, sample_buffer_right, &intensity_right);
    _audio3_process[AUDIO_VIRT ]->process_samples(sample_count, sample_buffer_virt , &intensity_virt );

    // Change power level based on audio level. Power level changes are slow, so do this at most once every 20ms
    static uint64_t last_level_change_time_us = 0;
    if (time_us_64() - last_level_change_time_us > (1000 * 20))
    {
        if 
        (
            last_intensity_left  != intensity_left  || 
            last_intensity_right != intensity_right ||
            last_intensity_virt  != intensity_virt 
        )
        {
            last_intensity_left  = intensity_left;
            last_intensity_right = intensity_right;
            last_intensity_virt  = intensity_virt;
            last_level_change_time_us = time_us_64();

            if (_routine_output)
            {
                _routine_output->audio_intensity_change (intensity_left, intensity_right, intensity_virt);
            }  
        }
    }

    free(sample_buffer_virt);
    _interuptable_section.end();
}

void CAudio::audio_intensity(uint16_t sample_count, uint8_t *sample_buffer_left, uint8_t *sample_buffer_right)
{
    _interuptable_section.start();
    static uint8_t last_intensity_left  = 0;
    static uint8_t last_intensity_right = 0;
    uint8_t intensity_left;
    uint8_t intensity_right;

    get_intensity(sample_count, sample_buffer_left , &intensity_left );
    get_intensity(sample_count, sample_buffer_right, &intensity_right);
        
    static uint64_t last_level_change_time_us = 0;
    if (time_us_64() - last_level_change_time_us > (1000 * 20))
    {
        if 
        (
            last_intensity_left  != intensity_left || 
            last_intensity_right != intensity_right
        )
        {
            last_intensity_left  = intensity_left;
            last_intensity_right = intensity_right;
            last_level_change_time_us = time_us_64();

            if (_routine_output)
            {
                _routine_output->audio_intensity_change (intensity_left, intensity_right);
            }  
        }
    }

    _interuptable_section.end();
}

void CAudio::get_intensity(uint16_t sample_count, uint8_t *buffer, uint8_t *out_intensity)
{
    // Figure out min, max and average (mean) sample values
    uint32_t total = 0;
    int16_t min = INT16_MAX;
    int16_t max = INT16_MIN;
    for (uint16_t x = 0; x < sample_count; x++)
    {
        if (buffer[x] > max)
            max = buffer[x];

        if (buffer[x] < min)
            min = buffer[x];
    
        total += buffer[x];
    }
    uint8_t avg = total / sample_count;


    // Noise filter. If no/very weak signal, don't continue (don't want to trigger on noise)
    if (abs(min-avg) < 5 && abs(max-avg) < 5)
    {
        *out_intensity = 0;
        return;
    }

    *out_intensity = max - min; // crude approximation of volume
}

uint8_t* CAudio::get_virtual_channel(uint16_t sample_count, uint8_t *sample_buffer_left, uint8_t *sample_buffer_right)
{
    uint8_t *sample_buffer_virt = (uint8_t*)malloc(sample_count);
    if (!sample_buffer_virt)
    {
        printf("CAudio::get_virtual_channel: failed to allocate memory\n");
        return NULL;
    }

    for (uint16_t sample=0; sample < sample_count; sample++)
    {
        int16_t difference = sample_buffer_left[sample] - sample_buffer_right[sample];
        difference /= 2;

        int16_t virt = difference + 127;
        if (virt > 255)
            virt = 255;
        if (virt < 0)
            virt = 0;

        sample_buffer_virt[sample] = virt;
    }

    return sample_buffer_virt;
}
