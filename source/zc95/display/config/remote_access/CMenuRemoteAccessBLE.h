#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../CSavedSettings.h"
#include "../../../Bluetooth/CBluetooth.h"
#include "../../../RemoteAccess/CBtGatt.h"
#include "../../../core1/CRoutineOutput.h"

class CMenuRemoteAccessBLE : public CMenu
{
    public:
        CMenuRemoteAccessBLE(
            CDisplay* display,
            CGetButtonState *buttons, 
            CSavedSettings *saved_settings,
            CRoutineOutput *routine_output,
            std::vector<CRoutines::Routine> &routines,
            CRadio *radio);
        ~CMenuRemoteAccessBLE();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        struct display_area _area;

        CDisplay* _display;
        display_area _disp_area;
        CGetButtonState *_buttons;
        CSavedSettings *_saved_settings;
        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine> *_routines;
        CRadio *_radio;
        CBtGatt *_gatt_server;

};
