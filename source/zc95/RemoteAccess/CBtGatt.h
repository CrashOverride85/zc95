#ifndef _CBTGATT_H
#define _CBTGATT_H

#include <inttypes.h>
#include <stdio.h>

#include "btstack.h"
#include "ble/gatt-service/battery_service_server.h"

#include "../config.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutines.h"

class CBtGatt
{
    public:
        CBtGatt(CRoutineOutput *routine_output, std::vector<CRoutines::Routine> *routines);
        ~CBtGatt();

        void init();


    private:
        btstack_packet_callback_registration_t _hci_event_callback_registration;

        static uint16_t s_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
        static int s_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
        static void s_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

        void process_pulse_stream_message(uint8_t *buffer, uint16_t buffer_size);

        int8_t find_routine_by_name(std::string name);
        uint8_t pulse_message_for_channel(uint8_t channel, uint8_t channel_polarity);

        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine> *_routines;
        uint64_t _start_time_us = 0;
};

#endif
