#ifndef _CBLUETOOTHSCAN_H
#define _CBLUETOOTHSCAN_H


#include <vector>
#include <string>
#include <inttypes.h>

#include "pico/cyw43_arch.h"
#include "btstack.h"
#include "../CSavedSettings.h"


class CBluetoothScan
{
    public:
        struct bt_device_t 
        {
            std::string name;
            bd_addr_t address;
            CSavedSettings::bt_device_type_t type;
        };

        CBluetoothScan();
        ~CBluetoothScan();

        void start();
        void stop();
        static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        void get_devices_found(std::vector<CBluetoothScan::bt_device_t>& devices);
        static std::string s_get_bt_type_string(CSavedSettings::bt_device_type_t type);
        
        
    private:
        void process_advertising_report(const uint8_t * adv_data, uint8_t adv_size, bd_addr_t address);
        void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        bool address_in_list(bd_addr_t address);

        std::vector<bt_device_t> _devices_found;
        btstack_packet_callback_registration_t _hci_event_callback_registration;
        bool _started = false;
};

#endif