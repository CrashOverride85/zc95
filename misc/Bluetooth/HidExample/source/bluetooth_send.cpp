#include "bluetooth_send.h"
#include "btstack_defines.h"
#include "gatt.h"
#include <stdio.h>
#include <stdlib.h>

// Configured to send 1 32-bit value as usage page = 0xFF00 (in "Vendor-defined" range), usage = 0x01
const uint8_t ReportDescriptor[] = {
    0x06, 0x00, 0xff,              // USAGE_PAGE (Vendor Defined Page 1))
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // Collection (Application)
    0x85, 0x01,                    //   Report ID 1
    0x09, 0x01,                    //   USAGE (Vendor Usage 1)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x27, 0x90, 0xd0, 0x03, 0x00,  //   LOGICAL_MAXIMUM (250000)
    0x75, 0x40,                    //   Report Size (32)
    0x95, 0x01,                    //   Report Count (1)
    0x81, 0x02,                    //   INPUT
    0xc0                           // END_COLLECTION
};

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    // Name
    0x0a, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'Z', 'c', ' ', 'H', 'i', 'd', ' ', 'E', 'x',
    // 16-bit Service UUIDs
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE & 0xff, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE >> 8,
    // Appearance HID - Generic Human Interface Device
    0x03, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC0, 0x03,
};
const uint8_t adv_data_len = sizeof(adv_data);

static BluetoothSend *_s_BluetoothSend;

BluetoothSend::BluetoothSend()
{
    _s_BluetoothSend = this;
}

void BluetoothSend::setup()
{
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
    gatt_client_set_required_security_level(LEVEL_2);
    att_server_init(profile_data, NULL, NULL);
    gatt_client_init();   
    hids_device_init(0, ReportDescriptor, sizeof(ReportDescriptor));

    uint16_t adv_int_min = 0x0300;
    uint16_t adv_int_max = 0x0300;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);

    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
    gap_advertisements_enable(1);

    _event_callback_registration.callback = &BluetoothSend::s_packet_handler;
    hci_add_event_handler(&_event_callback_registration);
    sm_add_event_handler (&_event_callback_registration);
    l2cap_add_event_handler(&_event_callback_registration);
    att_server_register_packet_handler(&BluetoothSend::s_packet_handler);
    hids_device_register_packet_handler(&BluetoothSend::s_packet_handler);

    hci_power_control(HCI_POWER_ON);
}

void BluetoothSend::s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (_s_BluetoothSend != NULL)
        _s_BluetoothSend->packet_handler(packet_type, channel, packet, size);
}

void BluetoothSend::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (packet_type != HCI_EVENT_PACKET)
        return;

    uint16_t conn_interval;

    switch (hci_event_packet_get_type(packet)) 
    {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            _con_handle = HCI_CON_HANDLE_INVALID;
            printf("Disconnected\n");
            _connected = false;
            break;
        case SM_EVENT_JUST_WORKS_REQUEST:
            printf("Just Works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            printf("L2CAP Connection Parameter Update Complete, response: %x\n", l2cap_event_connection_parameter_update_response_get_result(packet));
            break;
        case HCI_EVENT_META_GAP:
        {
            uint8_t gap_event = hci_event_gap_meta_get_subevent_code(packet);
            printf("HCI_EVENT_META_GAP = %d\n", gap_event);
            break;
        }
        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                    // print connection parameters (without using float operations)
                    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
                    printf("LE Connection Update:\n");
                    printf("- Connection Interval: %u.%02u ms\n", conn_interval * 125 / 100, 25 * (conn_interval & 3));
                    printf("- Connection Latency: %u\n", hci_subevent_le_connection_update_complete_get_conn_latency(packet));
                    break;
                default:
                    break;
            }
            break;  

        case HCI_EVENT_HIDS_META:
            printf("HID packet\n");
            switch (hci_event_hids_meta_get_subevent_code(packet)){
                case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
                    _con_handle = hids_subevent_input_report_enable_get_con_handle(packet);
                    printf("Report Characteristic Subscribed %u\n", hids_subevent_input_report_enable_get_enable(packet));   
                    _connected = true;
                    break;

                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}

void BluetoothSend::send(uint32_t val)
{
    if (_con_handle == HCI_CON_HANDLE_INVALID)
    {
        // printf("not connected.\n");
        return;
    }
    
    if (val != _last_value_sent)
    {
        _last_value_sent = val;
        printf("send %lu\n", val);
        hids_device_send_input_report(_con_handle, (uint8_t*) &val, sizeof(val));
    }
}

bool BluetoothSend::is_connected()
{
    return ((_con_handle != HCI_CON_HANDLE_INVALID) && _connected);
}
