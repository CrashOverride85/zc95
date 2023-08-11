#include "CBluetoothPair.h"
#include "CBluetoothPairGatt.h"
#include "pico/async_context_poll.h"

static CBluetoothPair *_s_CBluetoothPair;

CBluetoothPair::CBluetoothPair()
{
    _s_CBluetoothPair = this;
    _started = false;
}

CBluetoothPair::~CBluetoothPair()
{
    stop();
    _s_CBluetoothPair = NULL;
}

void CBluetoothPair::set_address(bd_addr_t address)
{
    memcpy(_address, address, BD_ADDR_LEN);
}

void CBluetoothPair::start()
{
    if (!_started)
    {
        printf("CBluetoothPair::start()\n");
        _started = true;

        l2cap_init();
        sm_init();
        sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
        sm_set_authentication_requirements(SM_AUTHREQ_BONDING);
        gatt_client_set_required_security_level(LEVEL_2);

        att_server_init(profile_data, NULL, NULL);
        gatt_client_init();

        _event_callback_registration.callback = &CBluetoothPair::s_packet_handler;
        hci_add_event_handler(&_event_callback_registration);
        sm_add_event_handler (&_event_callback_registration);
        gap_connect(_address, BD_ADDR_TYPE_LE_PUBLIC);
    }
}

void CBluetoothPair::stop()
{
    if (_started)
    {
        printf("CBluetoothPair::stop()\n");
        
        hci_remove_event_handler(&_event_callback_registration);
      //gatt_client_deinit(); MISSING FROM API?!
        att_server_deinit();
        sm_deinit();
        l2cap_deinit();

        _started = false;
    }
}

void CBluetoothPair::s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (_s_CBluetoothPair != NULL)
        _s_CBluetoothPair->packet_handler(packet_type, channel, packet, size);
}

void CBluetoothPair::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (packet_type != HCI_EVENT_PACKET) 
        return;

    /* bd_addr_t address;
    uint8_t length;
    const uint8_t *data;*/

    switch (hci_event_packet_get_type(packet)) 
    {
        case HCI_EVENT_LE_META:
            {
                hci_con_handle_t con_handle;
                if (hci_event_le_meta_get_subevent_code(packet) != HCI_SUBEVENT_LE_CONNECTION_COMPLETE) break;
                con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                printf("CBluetoothPair::packet_handler: connected\n");
                sm_request_pairing(con_handle);
            }
            break;
        
        case SM_EVENT_JUST_WORKS_REQUEST:
            printf("CBluetoothPair::packet_handler: Just works requested\n");
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;

        case SM_EVENT_PAIRING_STARTED:
            printf("CBluetoothPair::packet_handler: Pairing started\n");
            break;

        case SM_EVENT_PAIRING_COMPLETE:
            switch (sm_event_pairing_complete_get_status(packet))
            {
                case ERROR_CODE_SUCCESS:
                    printf("Pairing complete, success\n");
                    break;
                case ERROR_CODE_CONNECTION_TIMEOUT:
                    printf("Pairing failed, timeout\n");
                    break;
                case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                    printf("Pairing failed, disconnected\n");
                    break;
                case ERROR_CODE_AUTHENTICATION_FAILURE:
                    printf("Pairing failed, authentication failure with reason = %u\n", sm_event_pairing_complete_get_reason(packet));
                    break;
                default:
                    break;
            }
            break;

        case SM_EVENT_REENCRYPTION_STARTED:
            {
                bd_addr_t addr;
                sm_event_reencryption_complete_get_address(packet, addr);
                printf("Bonding information exists for addr type %u, identity addr %s -> start re-encryption\n",
                    sm_event_reencryption_started_get_addr_type(packet), bd_addr_to_str(addr));
            }
            break;

        case SM_EVENT_REENCRYPTION_COMPLETE:
            {
                bd_addr_t addr;
                bd_addr_type_t addr_type;

                switch (sm_event_reencryption_complete_get_status(packet))
                {
                    case ERROR_CODE_SUCCESS:
                        printf("Re-encryption complete, success\n");
                        break;

                    case ERROR_CODE_CONNECTION_TIMEOUT:
                        printf("Re-encryption failed, timeout\n");
                        break;

                    case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
                        printf("Re-encryption failed, disconnected\n");
                        break;

                    case ERROR_CODE_PIN_OR_KEY_MISSING:
                        printf("Re-encryption failed, bonding information missing\n\n");
                        printf("Assuming remote lost bonding information\n");
                        printf("Deleting local bonding information and start new pairing...\n");
                        sm_event_reencryption_complete_get_address(packet, addr);
                        addr_type = (bd_addr_type_t)sm_event_reencryption_started_get_addr_type(packet);
                        gap_delete_bonding(addr_type, addr);
                        sm_request_pairing(sm_event_reencryption_complete_get_handle(packet));
                        break;

                    default:
                        break;
                }
            }
            break;

        default:
            break;
    }
}
