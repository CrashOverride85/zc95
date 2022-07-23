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
    uint8_t *audio_samples;
    display_area disp_area = _display->get_display_area();

    float scale_factor = (float)255 / (float)((float)_bar_graph_area.y0-(float)disp_area.y0);

    uint16_t samples_count = 0;
    _audio->get_audio_buffer(CAnalogueCapture::channel::LEFT, &samples_count, &audio_samples);

    uint8_t y_middle = disp_area.y0+(_bar_graph_area.y0-disp_area.y0)/2;

    for (int x = 0; x+1 < disp_area.x1; x++)
    {
        float y0 = (float)disp_area.y0+((float)audio_samples[x]/scale_factor);
        float y1 = (float)disp_area.y0+((float)audio_samples[x+1]/scale_factor);

        hagl_draw_line(x, y0, x+1, y1, 0xFF);
    }

    // Line in centre of waveform
    hagl_draw_line(disp_area.x0, y_middle, disp_area.x1, y_middle, 0xAA);

    color_t bar_colour = hagl_color(0x00, 0x00, 0xFF);
   _bar_graph->draw_horz_bar_graph(_bar_graph_area, 0, 255, _gain, "vol", bar_colour);
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
    _audio->set_gain(CAnalogueCapture::channel::LEFT,  gain);
    _audio->set_gain(CAnalogueCapture::channel::RIGHT, gain);

    if (save)
    {
        _saved_settings->set_audio_gain_left (gain);
        _saved_settings->set_audio_gain_right(gain);
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
