#ifndef _CMENUSETTINGAUDIO_H
#define _CMENUSETTINGAUDIO_H

#include "CMenu.h"
#include "CDisplay.h"
#include "CHorzBarGraph.h"
#include "../CSavedSettings.h"
#include "../AudioInput/CAudio.h"

class CMenuSettingAudio : public CMenu
{
    public:
        CMenuSettingAudio(CDisplay* display, CGetButtonState *buttons, CAudio *audio, CSavedSettings *saved_settings);
        ~CMenuSettingAudio();
        void button_pressed(Button button);
        void draw();
        void show();
        void adjust_rotary_encoder_change(int8_t change);

    private:
        void set_gain(uint8_t gain, bool save);
        bool mic_power_enabled();
        bool mic_preamp_enabled();
        void set_menu_labels();
        bool show_stereo();
        CDisplay* _display;
        CGetButtonState *_buttons;
        CAudio *_audio;
        CHorzBarGraph *_bar_graph = NULL;
        struct display_area _bar_graph_area;
        uint8_t _gain = 130;
        CSavedSettings *_saved_settings;
};

#endif
