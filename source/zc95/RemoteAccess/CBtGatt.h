#ifndef _CBTGATT_H
#define _CBTGATT_H

#include <inttypes.h>
#include <stdio.h>

#include "btstack.h"
#include "ble_message.h"
#include "ble/gatt-service/battery_service_server.h"

#include "../config.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutines.h"

class CBtGatt
{
    public:
        CBtGatt(CRoutineOutput *routine_output, std::vector<CRoutines::Routine> const &routines);
        ~CBtGatt();

        void init(uint8_t bat_level_pc);
        void loop();
        void set_battery_percentage(uint8_t bat);


    private:
        struct channel_power_t
        {
            bool notifications_enabled;
            bool notification_pending;
            uint16_t power_level;
        };        

        btstack_packet_callback_registration_t _hci_event_callback_registration;

        static uint16_t s_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
        uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
        
        static int s_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
        int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

        static void s_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

        void send_notifications();
        void disconnected();
        void routine_run(bool run);

        uint8_t process_pulse_stream_packet(uint8_t *buffer, uint16_t buffer_size);
        void process_pulse_message(ble_message_pulse_t *msg);

        uint8_t process_set_channel_isolation_packet(uint8_t *buffer, uint16_t buffer_size);

        int8_t find_routine_by_name_and_set_config(std::string name);
        uint8_t pulse_message_for_channel(uint8_t channel, uint8_t channel_polarity);

        uint8_t set_channel_frequency(uint8_t channel, uint8_t *buffer, uint16_t buffer_size);
        uint8_t set_channel_pulse_width(uint8_t channel, uint8_t *buffer, uint16_t buffer_size);
        uint8_t set_channel_power(uint8_t channel, uint8_t *buffer, uint16_t buffer_size);

        uint8_t* allow_channel_isolation_disable();

        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine> _routines;
        uint64_t _start_time_us = 0;

        channel_power_t _channel_power_change[MAX_CHANNELS] = {0};
        uint64_t _last_power_status_update_us = 0;
        uint8_t _battery_percentage = 0;
        bool _routine_running = false;
        struct routine_conf _routine_conf;
        uint8_t _routine_id = 0;
        bool _init_ran = false;

        hci_con_handle_t _bt_connection_handle = HCI_CON_HANDLE_INVALID;

        // debug counters
        uint32_t _dbg_missing_packet_counter = 0;
        uint32_t _dbg_fifo_full_counter = 0;
};

#endif
