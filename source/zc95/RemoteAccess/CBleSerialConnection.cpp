/*
 * ZC95
 * Copyright (C) 2026  CrashOverride85
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

/*
 * As CSerialConnection, but over BLE using the Nordic UART service (NUS) rather than the
 * Aux/Serial port. A BLE disconnect is treated the same as an EOT.
 */

#include "CBleSerialConnection.h"
#include "ble/gatt-service/nordic_spp_service_server.h"

// Generated from BleSerialProfile.gatt; renamed to not clash with GattProfile.h
#define profile_data ble_serial_profile_data
#include "BleSerialProfile.h"
#undef profile_data

static CBleSerialConnection *_s_CBleSerialConnection = NULL;

static const uint8_t _adv_data[] =
{
    // Flags general discoverable, BR/EDR not supported
    2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,

    // Name
    5, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'Z', 'C', '9', '5',

    // Nordic UART service, 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
    17, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0, 0x93, 0xf3, 0xa3, 0xb5, 0x01, 0x00, 0x40, 0x6e,
};
static const uint8_t _adv_data_len = sizeof(_adv_data);

CBleSerialConnection::CBleSerialConnection(CRoutineOutput *routine_output, std::vector<CRoutines::Routine> &routines, CSdCard* sd_card) : _routines(routines)
{
    printf("CBleSerialConnection()\n");
    _s_CBleSerialConnection = this;
    _routine_output = routine_output;
    _sd_card = sd_card;

    memset(_recv_buffer, 0, sizeof(_recv_buffer));
    memset(_connected_device_address, 0, sizeof(_connected_device_address));

    _send_request.callback = &CBleSerialConnection::s_can_send_now;
    _send_request.context = this;

    _messageProcessor = new CMessageProcessor(_routine_output, _routines, std::bind(&CBleSerialConnection::send, this, std::placeholders::_1), _sd_card);
}

CBleSerialConnection::~CBleSerialConnection()
{
    printf("~CBleSerialConnection()\n");

    if (_init_ran)
    {
        gap_advertisements_enable(0);
        hci_remove_event_handler(&_hci_event_callback_registration);

        att_server_deinit();
        sm_deinit();
        l2cap_deinit();
        hci_power_control(HCI_POWER_OFF);
    }

    if (_messageProcessor != NULL)
    {
        delete _messageProcessor;
        _messageProcessor = NULL;
    }

    _s_CBleSerialConnection = NULL;
}

void CBleSerialConnection::init()
{
    if (_init_ran)
        return;

    l2cap_init();
    sm_init();

    att_server_init(ble_serial_profile_data, NULL, NULL);
    nordic_spp_service_server_init(&CBleSerialConnection::s_packet_handler);

    // setup advertisements
    uint16_t adv_int_min = 0x0300;
    uint16_t adv_int_max = 0x0300;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, sizeof(bd_addr_t));
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(_adv_data_len, (uint8_t*) _adv_data);
    gap_advertisements_enable(1);

    // register for HCI and ATT events
    _hci_event_callback_registration.callback = &CBleSerialConnection::s_packet_handler;
    hci_add_event_handler(&_hci_event_callback_registration);
    att_server_register_packet_handler(CBleSerialConnection::s_packet_handler);

    hci_power_control(HCI_POWER_ON);
    _init_ran = true;
}

void CBleSerialConnection::s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    // The Nordic UART service's handler stays registered after we're deleted
    if (_s_CBleSerialConnection != NULL)
        _s_CBleSerialConnection->packet_handler(packet_type, channel, packet, size);
}

void CBleSerialConnection::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (packet_type == RFCOMM_DATA_PACKET)
    {
        receive(packet, size);
        return;
    }

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet))
    {
        case ATT_EVENT_CONNECTED:
            printf("BLE serial: connected\n");
            _con_handle = att_event_connected_get_handle(packet);
            att_event_connected_get_address(packet, _connected_device_address);
            break;

        case ATT_EVENT_DISCONNECTED:
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            if (_con_handle != HCI_CON_HANDLE_INVALID)
            {
                printf("BLE serial: disconnected, reset connection\n");
                _con_handle = HCI_CON_HANDLE_INVALID;
                memset(_connected_device_address, 0, sizeof(_connected_device_address));
                _reset_connection = true;
            }
            break;
    }
}

void CBleSerialConnection::receive(uint8_t *data, uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        uint8_t ch = data[i];

        if (ch == EOT)
        {
            printf("EOT received, reset connection\n");
            _reset_connection = true;
        }

        if (_reset_connection)
            return;

        switch (_state)
        {
            case state_t::IDLE:
                if (ch == STX)
                    _state = state_t::RECV;
                break;

            case state_t::RECV:
                if ((_recv_buffer_position > MAX_WS_MESSAGE_SIZE) || (_recv_buffer_position > sizeof(_recv_buffer)-2))
                {
                    printf("CBleSerialConnection::receive(): max buffer length reached, discarding message\n");
                    _recv_buffer_position = 0;
                    memset(_recv_buffer, 0, sizeof(_recv_buffer));
                    _state = state_t::IDLE;
                }
                else
                {
                    if (ch == ETX)
                    {
                        printf("Got BLE serial message\n");
                        _state = state_t::GOT_MESSAGE;
                    }
                    else
                        _recv_buffer[_recv_buffer_position++] = ch;
                }
                break;

            case state_t::GOT_MESSAGE:
                break;
        }
    }
}

void CBleSerialConnection::loop()
{
    if (_state == state_t::GOT_MESSAGE)
    {
        _messageProcessor->message(_recv_buffer, _recv_buffer_position);
        _recv_buffer_position = 0;
        memset(_recv_buffer, 0, sizeof(_recv_buffer));
        _state = state_t::IDLE;
    }

    if (_messageProcessor)
        _messageProcessor->loop();

    if (_reset_connection)
        reset_message_processor();

    request_send();
}

void CBleSerialConnection::request_send()
{
    if (_tx_requested || _tx_buffer.empty() || _con_handle == HCI_CON_HANDLE_INVALID)
        return;

    _tx_requested = true;
    nordic_spp_service_server_request_can_send_now(&_send_request, _con_handle);
}

void CBleSerialConnection::reset_message_processor()
{
    printf("CBleSerialConnection: start reset\n");
    if (_messageProcessor)
        delete _messageProcessor;

    _recv_buffer_position = 0;
    memset(_recv_buffer, 0, sizeof(_recv_buffer));
    _state = state_t::IDLE;
    _tx_buffer.clear();
    _tx_requested = false;

    _messageProcessor = new CMessageProcessor(_routine_output, _routines, std::bind(&CBleSerialConnection::send, this, std::placeholders::_1), _sd_card);
    _reset_connection = false;
    printf("CBleSerialConnection: reset done\n");
}

void CBleSerialConnection::send(std::string message)
{
    printf("send > %s\n", message.c_str());

    if (_con_handle == HCI_CON_HANDLE_INVALID)
        return;

    if (_tx_buffer.length() + message.length() + 2 > SERIAL_TX_QUEUE_SIZE)
    {
        printf("CBleSerialConnection::send() error - not enough space in buffer (%d message size vs used %d)\n", message.length(), _tx_buffer.length());
        return;
    }

    _tx_buffer += (char)STX;
    _tx_buffer += message;
    _tx_buffer += (char)ETX;

    request_send();
}

void CBleSerialConnection::s_can_send_now(void *context)
{
    ((CBleSerialConnection*)context)->can_send_now();
}

void CBleSerialConnection::can_send_now()
{
    _tx_requested = false;
    if (_con_handle == HCI_CON_HANDLE_INVALID || _tx_buffer.empty())
        return;

    // Notifications larger than one link layer packet were being lost when sent back to back
    uint16_t chunk = att_server_get_mtu(_con_handle) - 3;
    if (chunk > 244)
        chunk = 244;
    if (chunk > _tx_buffer.length())
        chunk = _tx_buffer.length();

    if (nordic_spp_service_server_send(_con_handle, (const uint8_t*)_tx_buffer.data(), chunk) == ERROR_CODE_SUCCESS)
        _tx_buffer.erase(0, chunk);
}

bool CBleSerialConnection::is_connected()
{
    return (_con_handle != HCI_CON_HANDLE_INVALID);
}

const char *CBleSerialConnection::get_connected_device_address()
{
    if (is_connected())
        return bd_addr_to_str(_connected_device_address);
    else
        return "";
}
