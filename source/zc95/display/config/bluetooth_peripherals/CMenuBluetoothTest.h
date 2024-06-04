#include "../../CMenu.h"
#include "../../CDisplay.h"
#include "../../COptionsList.h"
#include "../../../CSavedSettings.h"
#include "../../../Bluetooth/CBluetooth.h"

#include "pico/util/queue.h"
#include <deque>

class CMenuBluetoothTest : public CMenu
{
    public:
        CMenuBluetoothTest(CDisplay* display, CBluetooth *bluetooth);

        ~CMenuBluetoothTest();
        void button_pressed(Button button);
        void adjust_rotary_encoder_change(int8_t change);
        void draw();
        void show();

    private:
        void draw_bt_remote();
        void draw_bt_other();

        display_area _disp_area;
        queue_t _bt_keypress_queue = {0};
        std::string _remote_message;
        uint64_t _keypress_displayed_us;

        CDisplay* _display;
        CBluetooth *_bluetooth;
        bool _hid_view = true;
        std::deque<CBluetoothConnect::bt_raw_hid_queue_entry_t> _hid_display_items;
        
};
