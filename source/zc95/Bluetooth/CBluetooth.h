#ifndef _CBLUETOOTH_H
#define _CBLUETOOTH_H


#include <vector>
#include <string>
#include <inttypes.h>

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

        void scan_get_devices_found(std::vector<CBluetoothScan::bt_device_t>& devices);
        void pair(bd_addr_t address);
        void connect(bd_addr_t address);
        bool is_paired(bd_addr_t address);
        
//        static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        


    private:
        CRadio *_radio;
        state_t _state = state_t::OFF;
        CBluetoothScan      _cBluetoothScan;
        CBluetoothPair      _cBluetoothPair;
        CBluetoothConnect   _cBluetoothConnect;
        // void connect(bd_addr_t address);

};

#endif