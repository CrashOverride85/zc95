#ifndef _CBLESERIALCONNECTION_H
#define _CBLESERIALCONNECTION_H

#include <inttypes.h>
#include <stdio.h>
#include <string>

#include "btstack.h"

#include "CMessageProcessor.h"
#include "../../common/zc95_config.h"
#include "../core1/CRoutineOutput.h"
#include "../core1/routines/CRoutines.h"
#include "../SdCard/CSdCard.h"

class CBleSerialConnection
{
    public:
        CBleSerialConnection(CRoutineOutput *routine_output, std::vector<CRoutines::Routine> &routines, CSdCard* sd_card);
        ~CBleSerialConnection();
        void init();
        void send(std::string message);
        void loop();
        bool is_connected();
        const char *get_connected_device_address();

    private:
        static void s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
        static void s_can_send_now(void *context);
        void can_send_now();
        void request_send();
        void receive(uint8_t *data, uint16_t size);
        void reset_message_processor();

        const uint8_t STX = 0x02;
        const uint8_t ETX = 0x03;
        const uint8_t EOT = 0x04;

        enum class state_t
        {
            IDLE,        // Waiting for STX
            RECV,        // STX received, getting data
            GOT_MESSAGE, // ETX received, got message ready to be processed
        };

        CRoutineOutput *_routine_output;
        std::vector<CRoutines::Routine>& _routines;
        CSdCard* _sd_card;

        CMessageProcessor *_messageProcessor = NULL;
        state_t _state = state_t::IDLE;
        bool _reset_connection = false;
        bool _init_ran = false;

        uint8_t _recv_buffer[MAX_WS_MESSAGE_SIZE+3]; // +2 for STX/ETX, +1 for NULL
        uint16_t _recv_buffer_position = 0;

        std::string _tx_buffer;
        bool _tx_requested = false;
        btstack_context_callback_registration_t _send_request;

        btstack_packet_callback_registration_t _hci_event_callback_registration;
        hci_con_handle_t _con_handle = HCI_CON_HANDLE_INVALID;
        bd_addr_t _connected_device_address;
};

#endif
