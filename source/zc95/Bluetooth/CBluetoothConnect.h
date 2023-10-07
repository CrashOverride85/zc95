#ifndef _CBluetoothConnect_H
#define _CBluetoothConnect_H


#include <vector>
#include <string>
#include <inttypes.h>

#include "CBluetoothRemote.h"

#include "pico/cyw43_arch.h"
#include "btstack.h"

class CBluetoothConnect
{
    public:
        enum bt_connect_state_t
        {
            STOPPED,
            IDLE,
            CONNECTING,
            CONNECTED,
            DISCONNECTED
        };

        CBluetoothConnect();
        ~CBluetoothConnect();

        void start();
        void stop();
        
        void set_address(bd_addr_t address);
        void get_address(bd_addr_t *address);

        static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        static void s_loop(btstack_timer_source_t *ts);
        
    private:
        void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        void hid_handle_input_report(uint8_t service_index, const uint8_t * report, uint16_t report_len);
        void set_state(bt_connect_state_t new_state);
        void connect();
        void loop(btstack_timer_source_t *ts);

        const uint32_t _loop_time_ms = 200;
        btstack_packet_callback_registration_t _event_callback_registration;
        bd_addr_t _address;
        bt_connect_state_t _state;
        uint64_t _last_state_change;

        hci_con_handle_t _connection_handle;
        uint8_t _hid_descriptor_storage[500];
        uint16_t _hids_cid;
        btstack_timer_source_t _timer;
        CBluetoothRemote _bluetooth_remote;
        
};

#endif