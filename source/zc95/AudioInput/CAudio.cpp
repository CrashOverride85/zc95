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
}

void CAudio::get_audio_buffer(CAnalogueCapture::channel chan, uint16_t *samples, uint8_t **buffer)
{
    _analogueCapture->get_audio_buffer(chan, samples, buffer);
}

void CAudio::set_gain(CAnalogueCapture::channel chan, uint8_t value)
{
    if (chan == CAnalogueCapture::channel::LEFT)
        _mcp4651->set_val(0, 255-value);
    else
        _mcp4651->set_val(1, 255-value);
}

void CAudio::mic_preamp_enable(bool enable)
{
    _controlsPortExp->mic_preamp_enable(enable);
}

void CAudio::mic_power_enable(bool enable)
{
    _controlsPortExp->mic_power_enable(enable);
}

void CAudio::init(CSavedSettings *saved_settings)
{
    set_gain(CAnalogueCapture::channel::LEFT,  saved_settings->get_audio_gain_left ());
    set_gain(CAnalogueCapture::channel::RIGHT, saved_settings->get_audio_gain_right());
    mic_power_enable (saved_settings->get_mic_power_enabled ());
    mic_preamp_enable(saved_settings->get_mic_preamp_enabled());
}

void CAudio::set_routine_output(CRoutineOutput *routine_output)
{
    _routine_output = routine_output;
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

void CAudio::draw_mic_audio_wave(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    _max_trigger_point = y1 - y0;
    if (_trigger_point > _max_trigger_point)
        _trigger_point = _max_trigger_point;

    draw_mic_audio_wave_v3(x0, y0, x1, y1);

    hagl_draw_rectangle(x0, y0, x1, y1, hagl_color(0x00, 0x00, 0xFF));    

    color_t colour = hagl_color(0xFF, 0x00, 0x00);
    hagl_draw_line(x0, y1-_trigger_point, x1, y1-_trigger_point, colour);
}
        

void CAudio::draw_mic_audio_wave_v1(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint16_t sample_count;
    uint8_t *sample_buffer;
   _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::LEFT, &sample_count, &sample_buffer);

    uint8_t width  = x1 - x0;
    uint8_t height = y1 - y0;

    color_t colour = hagl_color(0x00, 0x00, 0xFF);

    hagl_draw_line(x0, y0, x1, y1, hagl_color(0xFF, 0x00, 0x00));

    // Scale factor to scale 0-255 samples to whatever screen space is available
    float scale_factor = (float)128 / (float)height;

    // process audio - get averge value
    uint32_t total = 0;
    for (int i=0; i < sample_count; i++)
        total += sample_buffer[i];

//    uint32_t average = 128; 
      uint32_t average = total / sample_count;





    uint8_t sample_number=0;
    uint8_t max=0;
    for (uint8_t x=x0; x < x0+width; x++)
    {
        if ((sample_buffer[sample_number] > max) && sample_number > 2)
            max = sample_buffer[sample_number];

        int32_t y_no_dc = sample_buffer[sample_number] - average;
        float plot_y = y_no_dc < 0 ? 0 : y_no_dc;
        if (plot_y > 128)
            plot_y = 128;

        plot_y = plot_y / scale_factor;
        hagl_put_pixel(x, (float)y1 - plot_y, colour);

        if (sample_number == 0)
        {
            printf("plot_y=%f, ", plot_y);
        }

        sample_number++;
    }

    printf("sample count=%d, width=%d, height=%d, y0=%d, y1=%d, max=%d\n", sample_count, width, height, y0, y1, max);


}


void CAudio::draw_mic_audio_wave_v2(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint16_t sample_count;
    uint8_t *sample_buffer;
   _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::LEFT, &sample_count, &sample_buffer);

    uint8_t width  = x1 - x0;
    uint8_t height = y1 - y0;

    uint8_t group_a_size_in_samples = ((float)sample_count/(float)width)+1;
    uint8_t group_b_size_in_samples = (float)sample_count/(float)width;

    uint16_t group_a_count = sample_count % width;
    uint16_t group_b_count = width - group_a_count;

    color_t colour = hagl_color(0x00, 0x00, 0xFF);
    float scale_factor = (float)128 / (float)height;

     uint8_t limit = 20;

    uint16_t sample_pos=0;
    for (uint16_t x=0; x < width; x++)
    {
        uint16_t bar_height = 0 ;
        if (x < group_a_count)
        {
            bar_height = get_bar_height(sample_buffer, sample_pos, group_a_size_in_samples);
            sample_pos += group_a_size_in_samples;
        }
        else
        {
            bar_height = get_bar_height(sample_buffer, sample_pos, group_b_size_in_samples);
            sample_pos += group_b_size_in_samples;
        }

        bar_height = (float)bar_height/scale_factor;

   
        if (bar_height > limit)
            colour = hagl_color(0xFF, 0x00, 0x00);
        else
            colour = hagl_color(0x00, 0xFF, 0x00);
        
        hagl_draw_line(x0+x, y1, x0+x, y1 - bar_height, colour);
        

    }

    colour = hagl_color(0xFF, 0x00, 0x00);
    hagl_draw_line(x0, y1-limit, x1, y1-limit, colour);

    
}

uint16_t CAudio::get_bar_height(uint8_t *buffer, uint16_t sample_pos, uint8_t sample_count)
{
    uint16_t average=0;

    for (uint16_t i=sample_pos; i < sample_pos+sample_count; i++)
    {
        average += abs(buffer[i]-128);
    }
    average = average/sample_count;

    return average;
}




#define FFT_N       256  // Must be a power of 2
#define SAMPLEFREQ  (SAMPLES_PER_SECOND/6)


void CAudio::draw_mic_audio_wave_v3(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint16_t sample_count;
    uint8_t *sample_buffer;
    _analogueCapture->get_audio_buffer(CAnalogueCapture::channel::LEFT, &sample_count, &sample_buffer);

    // These are too big to put on the stack, especially with the recursion used by ESP_fft
    //float fft_input[FFT_N];
    //float fft_output[FFT_N];    
    float *fft_input  = (float*)malloc(sizeof(float)*FFT_N);
    float *fft_output = (float*)malloc(sizeof(float)*FFT_N);

    for (int k = 0; k < FFT_N; k++)
        fft_input[k] = (float)sample_buffer[k];

    ESP_fft FFT(FFT_N, SAMPLEFREQ, FFT_REAL, FFT_FORWARD, fft_input, fft_output);

    // Execute transformation
    FFT.removeDC();
    FFT.hammingWindow();

    FFT.execute();
    FFT.complexToMagnitude();

 /*
    printf("Fundamental Freq : %f Hz\t Mag: %f g\n", FFT.majorPeakFreq(), (FFT.majorPeak()/10000)*2/FFT_N);

    for (int i=0; i< FFT_N/2; i++) {
        printf("%f:%f\n", FFT.frequency(i),fft_output[i]);
    }
 */
    uint8_t width  = x1 - x0;
    uint8_t height = y1 - y0;

    color_t colour = hagl_color(0x00, 0xFF, 0x00);
    float scale_factor = (float)2000 / (float)height;

    uint16_t bar_height = 0;
    float   max_value=0;
    uint8_t max_index=0;
    uint8_t thres_cross_count=0;
    for (uint8_t x=x0; x < x0+width; x++)
    {
        bar_height = fft_output[x];
        bar_height = (float)bar_height/scale_factor;
        if (bar_height > height)
            bar_height = height;

        if (bar_height > _trigger_point)
        {
            colour = hagl_color(0xFF, 0x00, 0x00);
            thres_cross_count++;
        }
        else
            colour = hagl_color(0x00, 0xFF, 0x00);

        hagl_draw_line(x0+x, y1, x0+x, y1 - bar_height, colour);

        if (fft_output[x] > max_value)
        {
            max_value = fft_output[x];
            max_index = x;
        }
    }

    if (thres_cross_count && _routine_output != NULL)
    {
        uint16_t freq = FFT.majorPeakFreq();
        printf("** FREQ = %d **\n", freq);
        _routine_output->audio_threshold_reached(FFT.majorPeakFreq(), thres_cross_count);
    }


    free(fft_input );
    free(fft_output);
}
