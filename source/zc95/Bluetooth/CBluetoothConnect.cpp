#include "CBluetoothConnect.h"
#include "CBluetoothPairGatt.h"
#include "pico/async_context_poll.h"

static CBluetoothConnect *_s_CBluetoothConnect;

CBluetoothConnect::CBluetoothConnect()
{
    _s_CBluetoothConnect = this;
    _state = bt_connect_state_t::STOPPED;
    _last_state_change = time_us_64();
    _bt_raw_hid_queue = NULL;
}

CBluetoothConnect::~CBluetoothConnect()
{
    stop();
    _s_CBluetoothConnect = NULL;
}

void CBluetoothConnect::set_keypress_queue(queue_t *bt_keypress_queue)
{
    _bluetooth_remote.set_keypress_queue(bt_keypress_queue);
}

void CBluetoothConnect::set_bt_raw_hid_queue(queue_t *bt_raw_hid_queue)
{
    _bt_raw_hid_queue = bt_raw_hid_queue;
}

void CBluetoothConnect::start()
{
    if (_state == bt_connect_state_t::STOPPED)
    {
        printf("CBluetoothConnect::start()\n");
        set_state(bt_connect_state_t::IDLE);

        _connection_handle = HCI_CON_HANDLE_INVALID;
        l2cap_init();
        sm_init();
        sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
        sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
        gatt_client_set_required_security_level(LEVEL_2);

        att_server_init(profile_data, NULL, NULL);
        gatt_client_init();
        hids_client_init(_hid_descriptor_storage, sizeof(_hid_descriptor_storage));

        _event_callback_registration.callback = &CBluetoothConnect::s_packet_handler;
        hci_add_event_handler(&_event_callback_registration);
        sm_add_event_handler (&_event_callback_registration);
    }
}

void CBluetoothConnect::stop()
{
    if (_state != bt_connect_state_t::STOPPED)
    {
        printf("bt: CBluetoothConnect::stop()\n");
        hids_client_disconnect(_hids_cid);
        hids_client_deinit();
        hci_remove_event_handler(&_event_callback_registration);
        sm_remove_event_handler(&_event_callback_registration);
        _connection_handle = HCI_CON_HANDLE_INVALID;

        att_server_deinit();
        sm_deinit();
        l2cap_deinit();
        set_state(bt_connect_state_t::STOPPED);
    }
}

void CBluetoothConnect::s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (_s_CBluetoothConnect != NULL)
        _s_CBluetoothConnect->packet_handler(packet_type, channel, packet, size);
}

void CBluetoothConnect::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (packet_type != HCI_EVENT_PACKET && packet_type != HCI_EVENT_GATTSERVICE_META) 
    {
        return;
    }

    uint8_t status;
    uint8_t hci_packet_type = hci_event_packet_get_type(packet);
    // printf("got hci packet type = 0x%x\n", hci_packet_type);

    switch (hci_packet_type) 
    {
        case BTSTACK_EVENT_STATE:
            connect();
            break;

        case HCI_EVENT_LE_META:
        {
            uint8_t meta_subevent_code = hci_event_le_meta_get_subevent_code(packet);
           // printf("meta_subevent_code = %d\n", meta_subevent_code);
            
            if (meta_subevent_code != HCI_SUBEVENT_LE_CONNECTION_COMPLETE) break;

            _connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
            printf("CBluetoothConnect::packet_handler: connected\n");
            sm_request_pairing(_connection_handle);
        }
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
             gap_stop_scan();
             break;

        case SM_EVENT_REENCRYPTION_COMPLETE:
            switch (sm_event_reencryption_complete_get_status(packet))
            {
                case ERROR_CODE_SUCCESS:
                    printf("bt: Re-encryption complete, success\n");
                    uint8_t hid_ret = hids_client_connect(_connection_handle, &CBluetoothConnect::s_packet_handler, HID_PROTOCOL_MODE_REPORT, &_hids_cid);
                    if (hid_ret == ERROR_CODE_COMMAND_DISALLOWED)
                    {
                        // This means that there's already a HID client connected, which almost certainly means this was a 
                        // reconnection, so we're good to go without needing to wait for GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED.
                        printf("bt: HID client already connected, setting state to CONNECTED\n");
                        set_state(bt_connect_state_t::CONNECTED);
                    }

            }
            break;

        case HCI_EVENT_GATTSERVICE_META:
            switch (hci_event_gattservice_meta_get_subevent_code(packet))
            {
                case GATTSERVICE_SUBEVENT_HID_SERVICE_CONNECTED:
                    status = gattservice_subevent_hid_service_connected_get_status(packet);
                    if (status == ERROR_CODE_SUCCESS)
                    {
                        printf("bt: HID service client connected\n");
                        set_state(bt_connect_state_t::CONNECTED);
                    }
                    else
                        printf("bt: HID service client connection failed, status 0x%02x.\n", status);
                    break;

                case GATTSERVICE_SUBEVENT_HID_REPORT:
                    hid_handle_input_report(
                        gattservice_subevent_hid_report_get_service_index(packet),
                        gattservice_subevent_hid_report_get_report(packet), 
                        gattservice_subevent_hid_report_get_report_len(packet));
                    break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("bt: disconnected!\n");
            set_state(bt_connect_state_t::DISCONNECTED);

        default:
            break;
    }
}

void CBluetoothConnect::hid_handle_input_report(uint8_t service_index, const uint8_t * report, uint16_t report_len)
{
    if (report_len < 1) return;

    btstack_hid_parser_t parser;

    btstack_hid_parser_init(&parser, 
        hids_client_descriptor_storage_get_descriptor_data(_hids_cid, service_index), 
        hids_client_descriptor_storage_get_descriptor_len(_hids_cid, service_index), 
        HID_REPORT_TYPE_INPUT, report, report_len);

    while (btstack_hid_parser_has_more(&parser))
    {
        uint16_t usage_page;
        uint16_t usage;
        int32_t  value;
        btstack_hid_parser_get_field(&parser, &usage_page, &usage, &value);

      //  printf("usage_page = 0x%x, usage = 0x%x, value = %li\n", usage_page, usage, value);
        if (_bt_device_type == CSavedSettings::bt_device_type_t::HID)
        {
            // shutter remotes
            _bluetooth_remote.process_input(usage_page, usage, value);
        }
        else
        {
            if (_bt_raw_hid_queue)
            {
                bt_raw_hid_queue_entry_t entry;
                entry.usage_page = usage_page;
                entry.usage = usage;
                entry.value = value;
                queue_try_add(_bt_raw_hid_queue, &entry);
            }
        }
    }

    /// printf("--------------------------------- \n");
}

void CBluetoothConnect::set_address(bd_addr_t address, CSavedSettings::bt_device_type_t type)
{
    memcpy(_address, address, BD_ADDR_LEN);
    _bt_device_type = type;
}

void CBluetoothConnect::get_address(bd_addr_t *address)
{
    memcpy(address, _address, BD_ADDR_LEN);
}

CBluetoothConnect::bt_connect_state_t CBluetoothConnect::get_state()
{
    return _state;
}

void CBluetoothConnect::connect()
{
    printf("bt: Connect to device with address %s\n", bd_addr_to_str(_address));
    set_state(bt_connect_state_t::CONNECTING);
    gap_connect(_address, BD_ADDR_TYPE_LE_PUBLIC);
}

void CBluetoothConnect::s_loop(btstack_timer_source_t *ts)
{
    ((CBluetoothConnect*)(ts->context))->loop(ts);
}

void CBluetoothConnect::loop(btstack_timer_source_t *ts)
{
    if (_state == bt_connect_state_t::DISCONNECTED && time_us_64() - _last_state_change > 1000 * 1000)
    {
        connect();
    }

    btstack_run_loop_set_timer(&_timer, _loop_time_ms);
    btstack_run_loop_set_timer_context(&_timer, this);
    btstack_run_loop_set_timer_handler(&_timer, &s_loop);
    btstack_run_loop_add_timer(&_timer);
}

void CBluetoothConnect::set_state(bt_connect_state_t new_state)
{
    if (new_state != _state)
    {
        // on start, setup a timer to call loop()
        if (_state == bt_connect_state_t::STOPPED && new_state != bt_connect_state_t::STOPPED)
        {
            printf("Start timer\n");
            btstack_run_loop_set_timer(&_timer, _loop_time_ms);
            btstack_run_loop_set_timer_context(&_timer, this);
            btstack_run_loop_set_timer_handler(&_timer, &s_loop);
            btstack_run_loop_add_timer(&_timer);
        }

        // on stop, remove the timer that calls loop()
        if (_state != bt_connect_state_t::STOPPED && new_state == bt_connect_state_t::STOPPED)
        {
            btstack_run_loop_remove_timer(&_timer);
        }

        _last_state_change = time_us_64();
        _state = new_state;
    }
}

std::string CBluetoothConnect::s_state_to_string(bt_connect_state_t state)
{
    std::string str_state;
    switch (state)
    {
        case bt_connect_state_t::CONNECTED:
            str_state = "Connected";
            break;

        case bt_connect_state_t::CONNECTING:
            str_state = "Connecting";
            break;

        case bt_connect_state_t::DISCONNECTED:
            str_state = "Disconnected";
            break;

        case bt_connect_state_t::IDLE:
            str_state = "Idle";
            break;

        case bt_connect_state_t::STOPPED:
            str_state = "Stopped";
            break;

        default:
            str_state = "<UNKNOWN>";
            break;
    }

    return str_state;
}
