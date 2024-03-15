#ifndef _CBLUETOOTHPAIR_H
#define _CBLUETOOTHPAIR_H


#include <vector>
#include <string>
#include <inttypes.h>

#include "pico/cyw43_arch.h"
#include "btstack.h"



class CBluetoothPair
{
    public:
        enum bt_pair_state_t
        {
            IDLE     = 0,
            START    = 1,
            SUCCESS  = 2,
            FAILED   = 3 
        };

        CBluetoothPair();
        ~CBluetoothPair();

        void set_address(bd_addr_t address);
        void get_address(bd_addr_t *address);
        void start();
        void stop();
        bt_pair_state_t get_state();
        static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        
    private:
        void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        void set_state(bt_pair_state_t newState);

        btstack_packet_callback_registration_t _event_callback_registration;
        bt_pair_state_t _state = bt_pair_state_t::IDLE;
        bd_addr_t _address;
       






};

#endif