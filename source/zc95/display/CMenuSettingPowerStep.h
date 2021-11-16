#ifndef _CMENUSETTINGPOWERSTEP_H
#define _CMENUSETTINGPOWERSTEP_H

#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "CHorzBarGraph.h"
#include "../CSavedSettings.h"
#include "../core1/output/COutputChannel.h"
#include "../core1/routines/CRoutine.h"

class CMenuSettingPowerStep : public CMenu
{
    public:
        CMenuSettingPowerStep(CDisplay* display, CGetButtonState *buttons, CSavedSettings *saved_settings);
        ~CMenuSettingPowerStep();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        struct power_level_step
        {
            std::string text;
            uint8_t step_interval;
        };

        power_level_step get_power_level_step(std::string text, uint8_t step_interval);
        void update_setting();

        COptionsList *_option_list = NULL;
        CDisplay* _display;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        uint8_t _power_step=10;
        std::vector<power_level_step> _power_level_steps;
};

#endif
