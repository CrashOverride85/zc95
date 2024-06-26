#ifndef _CBluetoothConnect_H
#define _CBluetoothConnect_H


#include <vector>
#include <string>
#include <inttypes.h>

#include "../CSavedSettings.h"
#include "CBluetoothRemote.h"

#include "pico/util/queue.h"
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

        struct bt_raw_hid_queue_entry_t
        {
            uint16_t usage_page;
            uint16_t usage;
            int32_t  value;
        };

        CBluetoothConnect();
        ~CBluetoothConnect();

        void start();
        void stop();
        
        void set_address(bd_addr_t address, CSavedSettings::bt_device_type_t type);
        void get_address(bd_addr_t *address);

        bt_connect_state_t get_state();
        
        void set_keypress_queue(queue_t *bt_keypress_queue);

        static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        static void s_loop(btstack_timer_source_t *ts);
        static std::string s_state_to_string(bt_connect_state_t state);
        
    private:
        void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        void hid_handle_input_report(uint8_t service_index, const uint8_t * report, uint16_t report_len);
        void set_state(bt_connect_state_t new_state);
        void connect();
        void loop(btstack_timer_source_t *ts);

        const uint32_t _loop_time_ms = 200;
        btstack_packet_callback_registration_t _event_callback_registration;
        bd_addr_t _address;
        CSavedSettings::bt_device_type_t _bt_device_type;
        bt_connect_state_t _state;
        uint64_t _last_state_change;

        hci_con_handle_t _connection_handle;
        uint8_t _hid_descriptor_storage[500];
        uint16_t _hids_cid;
        btstack_timer_source_t _timer;
        CBluetoothRemote _bluetooth_remote;
};

#endif
