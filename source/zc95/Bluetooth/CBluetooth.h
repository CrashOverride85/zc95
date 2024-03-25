#ifndef _CBLUETOOTH_H
#define _CBLUETOOTH_H


#include <vector>
#include <string>
#include <inttypes.h>

#include "pico/util/queue.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"

#include "../CRadio.h"
#include "CBluetoothScan.h"
#include "CBluetoothPair.h"
#include "CBluetoothConnect.h"

class CBluetooth
{
    public:
        enum state_t
        {
            OFF     = 0,
            SCAN    = 1,
            PAIR    = 2,
            CONNECT = 3
        };
        
        CBluetooth(CRadio *radio);
        ~CBluetooth();

        void set_state(state_t state);
        state_t get_state();
        CBluetoothPair::bt_pair_state_t get_pair_state();
        CBluetoothConnect::bt_connect_state_t get_connect_state();

        void scan_get_devices_found(std::vector<CBluetoothScan::bt_device_t>& devices);
        void set_keypress_queue(queue_t *bt_keypress_queue);
        void set_bt_raw_hid_queue(queue_t *bt_raw_hid_queue);
        void pair(bd_addr_t address, CSavedSettings::bt_device_type_t device_type);
        void connect(bd_addr_t address, CSavedSettings::bt_device_type_t device_type);
        bool is_paired(bd_addr_t address);

    private:
        CRadio *_radio;
        state_t _state = state_t::OFF;
        CBluetoothScan      _cBluetoothScan;
        CBluetoothPair      _cBluetoothPair;
        CBluetoothConnect   _cBluetoothConnect;
};

#endif