#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/output/CChannelConfig.h"
#include "../core1/routines/CRoutine.h"
#include "../core1/routines/CRoutines.h"

#include "../config.h"
#include "../CSavedSettings.h"
#include "../ECButtons.h"
#include "../CGetButtonState.h"
#include "../CHwCheck.h"
#include "../AudioInput/CAudio.h"

#include <string>
#include <vector>

#include "../core1/Core1.h"

class CMainMenu : public CMenu
{
    public:
        CMainMenu(CDisplay* display, std::vector<CRoutines::Routine> *routines, CGetButtonState *buttons, CSavedSettings *settings, CRoutineOutput *routine_output, CHwCheck *hwCheck, CAudio *audio, CAnalogueCapture *analogueCapture);
        ~CMainMenu();
        void button_pressed(Button button);
        void button_released(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        CDisplay* _display;
        std::vector<CRoutines::Routine> *_routines;
        CGetButtonState *_buttons;
        CSavedSettings *_settings;
};
