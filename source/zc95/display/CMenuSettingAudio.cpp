/*
 * ZC95
 * Copyright (C) 2022 CrashOverride85
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

#include "CMenuSettingAudio.h"
#include "../git_version.h"
#include "../globals.h"

CMenuSettingAudio::CMenuSettingAudio(CDisplay* display, CGetButtonState *buttons, CAudio *audio, CSavedSettings *saved_settings)
{
    printf("CMenuSettingAudio() \n");
    _display = display;
    _buttons = buttons;
    _audio = audio;
    _saved_settings = saved_settings;
    
    _bar_graph = new CHorzBarGraph(_display);
    display_area disp_area = _display->get_display_area();
    _bar_graph_area.x0 = disp_area.x0;
    _bar_graph_area.x1 = disp_area.x1 - 2;
    _bar_graph_area.y0 = disp_area.y1 - 25;
    _bar_graph_area.y1 = disp_area.y1;

    _gain = _saved_settings->get_audio_gain_left();
    set_gain(_gain, false);

    _audio->mic_power_enable(mic_power_enabled());
    _audio->mic_preamp_enable(mic_preamp_enabled());
}

CMenuSettingAudio::~CMenuSettingAudio()
{
    printf("~CMenuSettingAudio() \n");
    if (_bar_graph)
    {
        delete _bar_graph;
        _bar_graph = NULL;
    }
}

void CMenuSettingAudio::button_pressed(Button button)
{
    if (_submenu_active)
    {
        _submenu_active->button_pressed(button);
    }
    else
    {
        switch (button)
        {
            case Button::A: // Mic preamp enable/disable
                if (mic_preamp_enabled())
                {
                    // Disable mic preamp & mic power
                    _saved_settings->set_mic_preamp_enabled(false);
                    _saved_settings->set_mic_power_enabled(false);
                    _audio->mic_preamp_enable(false);
                    _audio->mic_power_enable(false);
                }
                else
                {
                    // enable mic preamp
                    _saved_settings->set_mic_preamp_enabled(true);
                    _audio->mic_preamp_enable(true);
                }
                
                set_menu_labels();
                break;

            case Button::B: // "Back"
                _exit_menu = true;
                break;

            case Button::C: // Mic power enable/disable
                
                // Don't allow mic power to be turned on without the mic preamp (pointless config combination)
                if (!mic_preamp_enabled())
                    break;

                if (mic_power_enabled())
                {
                    _saved_settings->set_mic_power_enabled(false);
                    _audio->mic_power_enable(false);
                }
                else
                {
                    _saved_settings->set_mic_power_enabled(true);
                    _audio->mic_power_enable(true);
                }
                
                set_menu_labels();
                break;

            default:
                break;
        }
    }
}

void CMenuSettingAudio::adjust_rotary_encoder_change(int8_t change)
{
    if (_submenu_active)
    {
        _submenu_active->adjust_rotary_encoder_change(change);
    }
    else
    {
        if (change >= 1)
        {
            if (_gain < 250)
            {
                _gain+=5;
            }
            else
            {
                _gain = 255;
            }
        }
        else if (change <= -1)
        {
            if (_gain > 5)
            {
                _gain -= 5;
            }
            else
            {
                _gain = 0;
            }
        }

        set_gain(_gain, true);
    }
}

void CMenuSettingAudio::draw()
{
    uint8_t *audio_samples_l;
    uint8_t *audio_samples_r;
    display_area disp_area = _display->get_display_area();

    // Scale factor to scale 0-255 samples to whatever screen space is available (~55 pixels?)
    float scale_factor = (float)255 / (float)((float)_bar_graph_area.y0-(float)disp_area.y0);

    uint16_t samples_count = 0;
    _audio->get_audio_buffer(CAnalogueCapture::channel::LEFT,  &samples_count, &audio_samples_l);
    _audio->get_audio_buffer(CAnalogueCapture::channel::RIGHT, &samples_count, &audio_samples_r);

    uint8_t y_middle = disp_area.y0+(_bar_graph_area.y0-disp_area.y0)/2;

    if (show_stereo())
    {
        // left
        for (int x = 0; x+1 < (disp_area.x1/2)-2; x++)
        {
            float y0 = (float)disp_area.y0+((float)audio_samples_l[x]/scale_factor);
            float y1 = (float)disp_area.y0+((float)audio_samples_l[x+1]/scale_factor);

            hagl_draw_line(_display->get_hagl_backed(), x, y0, x+1, y1, hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0xFF));
        }

        // right
        uint8_t sample=0;
        for (int x = (disp_area.x1/2)+2; x+1 < disp_area.x1; x++)
        {
            float y0 = (float)disp_area.y0+((float)audio_samples_r[sample]   / scale_factor);
            float y1 = (float)disp_area.y0+((float)audio_samples_r[sample+1] / scale_factor);
            sample++;

            hagl_draw_line(_display->get_hagl_backed(), x, y0, x+1, y1, hagl_color(_display->get_hagl_backed(), 0xFF, 0x00, 0x00));
        }
    }
    else
    {
        // If the mic power isn't on when it should be, the mic won't work.
        // Hopefully a different colour wave will make this much easier to spot.
        hagl_color_t colour;
        if (mic_power_enabled())
            colour = hagl_color(_display->get_hagl_backed(), 0xFF, 0x14, 0x93); // Pink, to match pc microphone socket, for electret microphones
        else
            colour = hagl_color(_display->get_hagl_backed(), 0xFF, 0xFF, 0x00); // Yellow, for dynamic mics (although not really enough gain for them to work well), or externally powered electret mics

        for (int x = 0; x+1 < disp_area.x1; x++)
        {
            float y0 = (float)disp_area.y0+((float)audio_samples_l[x]   / scale_factor);
            float y1 = (float)disp_area.y0+((float)audio_samples_l[x+1] / scale_factor);

            hagl_draw_line(_display->get_hagl_backed(), x, y0, x+1, y1, colour);
        }
    }

    // Line in centre of waveform
    hagl_draw_line(_display->get_hagl_backed(), disp_area.x0, y_middle, disp_area.x1, y_middle, 0xAA);

    // Only show gain control if audio mode set to AUTO and digipot was found
    if (_audio->get_audio_hardware_state() == audio_hardware_state_t::PRESENT)
    {
        hagl_color_t bar_colour = hagl_color(_display->get_hagl_backed(), 0x00, 0x00, 0xFF);
        _bar_graph->draw_horz_bar_graph(_bar_graph_area, 0, 255, _gain, "vol", bar_colour);
    }
}

void CMenuSettingAudio::show()
{
    _display->set_option_b("Back");
    _display->set_option_d(" ");

    set_menu_labels();

    _exit_menu = false;
}

void CMenuSettingAudio::set_gain(uint8_t gain, bool save)
{
    if (_audio->get_audio_hardware_state() == audio_hardware_state_t::PRESENT)
    {
        _audio->set_gain(CAnalogueCapture::channel::LEFT,  gain);
        _audio->set_gain(CAnalogueCapture::channel::RIGHT, gain);

        if (save)
        {
            _saved_settings->set_audio_gain_left (gain);
            _saved_settings->set_audio_gain_right(gain);
        }
    }
}

void CMenuSettingAudio::set_menu_labels()
{
    if (mic_preamp_enabled())
    {
        _display->set_option_a("Dsbl mic pre");

        if (mic_power_enabled())
        {
            _display->set_option_c("Dsbl mic pwr");
        }
        else
        {
            _display->set_option_c("Enbl mic pwr");
        }
    }
    else
    {
        _display->set_option_a("Enbl mic pre");
        _display->set_option_c(" ");
    }
}

bool CMenuSettingAudio::mic_power_enabled()
{
    return _saved_settings->get_mic_power_enabled();
}

bool CMenuSettingAudio::mic_preamp_enabled()
{
    return _saved_settings->get_mic_preamp_enabled();
}

bool CMenuSettingAudio::show_stereo()
{
    return !_saved_settings->get_mic_preamp_enabled();
}
