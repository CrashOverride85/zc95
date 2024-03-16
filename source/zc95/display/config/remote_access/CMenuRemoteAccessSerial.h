#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../CSavedSettings.h"
#include "../../../RemoteAccess/CWifi.h"
#include "../../../RemoteAccess/CSerialConnection.h"
#include "../../../core1/CRoutineOutput.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

class CMenuRemoteAccessSerial : public CMenu
{
    public:
        CMenuRemoteAccessSerial(
            CDisplay* display,
            CGetButtonState *buttons, 
            CSavedSettings *saved_settings,
            CRoutineOutput *routine_output,
            std::vector<CRoutines::Routine> *routines);
        ~CMenuRemoteAccessSerial();
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
        CSerialConnection *_serial_connection;
        std::vector<CRoutines::Routine> *_routines;
};
