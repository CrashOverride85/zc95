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
        CBluetoothPair();
        ~CBluetoothPair();

        void set_address(bd_addr_t address);
        void start();
        void stop();
        static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        //void get_devices_found(std::vector<CBluetoothPair::bt_device_t>& devices);
        
    private:
        //void process_advertising_report(const uint8_t * adv_data, uint8_t adv_size, bd_addr_t address);
        void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        //bool address_in_list(bd_addr_t address);

        btstack_packet_callback_registration_t _event_callback_registration;
        bool _started = false;
        bd_addr_t _address;
       






};

#endif