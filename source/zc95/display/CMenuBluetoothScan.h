#include "CMenu.h"
#include "CDisplay.h"
#include "COptionsList.h"
#include "../CSavedSettings.h"
#include "../CAnalogueCapture.h"
#include "../Bluetooth/CBluetooth.h"
#include "../RemoteAccess/CWifi.h"
#include "../core1/CRoutineOutput.h"

class CMenuBluetoothScan : public CMenu
{
    public:
        CMenuBluetoothScan(CDisplay* display, CSavedSettings *saved_settings, CBluetooth *bluetooth);

        ~CMenuBluetoothScan();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        void set_button_text();

        std::vector<CBluetoothScan::bt_device_t> _devices;

        int _last_selection = -1;
        COptionsList *_options_list = NULL;
        display_area _disp_area;
        
        CDisplay* _display;
        CBluetooth *_bluetooth;
};
