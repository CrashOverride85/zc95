#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../CSavedSettings.h"
#include "../../../Bluetooth/CBluetooth.h"
#include "../../../RemoteAccess/CBtGatt.h"
#include "../../../PowerManagement/IPowerManagement.h"
#include "../../../core1/CRoutineOutput.h"
#include "../../../Hal/IHal.h"

class CMenuRemoteAccessBLE : public CMenu
{
    public:
        CMenuRemoteAccessBLE(
            CDisplay* display,
            IHal *hal, 
            CSavedSettings *saved_settings,
            CRoutineOutput *routine_output,
            std::vector<CRoutines::Routine> &routines,
            CRadio *radio,
            IPowerManagement* power_management);
        ~CMenuRemoteAccessBLE();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        struct display_area _area;

        CDisplay* _display;
        display_area _disp_area;
        IHal *_hal;
        CSavedSettings *_saved_settings;
        CRoutineOutput *_routine_output;
        CRadio *_radio;
        CBtGatt *_gatt_server;
        IPowerManagement* _power_management;

};
