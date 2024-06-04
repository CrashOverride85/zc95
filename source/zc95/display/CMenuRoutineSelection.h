#ifndef _CMENUROUTINESELCTION_H
#define _CMENUROUTINESELCTION_H

#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "CMenuRoutineSelection.h"
#include "../core1/output/CFullChannelAsSimpleChannel.h"
#include "../core1/output/CChannelConfig.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutine.h"
#include "../core1/routines/CRoutines.h"
#include "../config.h"
#include "../CSavedSettings.h"
#include "../ECButtons.h"
#include "../CGetButtonState.h"
#include "../CHwCheck.h"
#include "../AudioInput/CAudio.h"
#include "../CAnalogueCapture.h"
#include "../RemoteAccess/CWifi.h"
#include "../Bluetooth/CBluetooth.h"

#include <string>
#include <vector>

#include "../core1/Core1.h"

class CMenuRoutineSelection : public CMenu
{
    public:
        CMenuRoutineSelection(
                CDisplay* display, 
                std::vector<CRoutines::Routine> &routines, 
                CGetButtonState *buttons, 
                CSavedSettings *settings, 
                CRoutineOutput *routine_output, 
                CHwCheck *hwCheck, 
                CAudio *audio, 
                CAnalogueCapture *analogueCapture, 
                CWifi *wifi, 
                CBluetooth *bluetooth,
                CRadio *radio);

        ~CMenuRoutineSelection();
        void button_pressed(Button button);
        void button_released(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        bool is_audio_routine(routine_conf conf);
        COptionsList *_routine_display_list = NULL;
        struct display_area _area;
        CDisplay* _display;
        std::vector<CRoutines::Routine> _routines;
        CGetButtonState *_buttons;
        CSavedSettings *_settings;
        CRoutineOutput *_routine_output;
        CHwCheck *_hwCheck;
        CAudio *_audio;
        int _last_selection = -1;
        CAnalogueCapture *_analogueCapture;
        CWifi *_wifi;
        CBluetooth *_bluetooth;
        CRadio *_radio;
};

#endif
