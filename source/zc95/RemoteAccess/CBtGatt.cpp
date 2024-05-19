#include "../globals.h"

#include "CBtGatt.h"
#include "GattProfile.h"
#include "ble_message.h"

#include "pico/util/queue.h"

static CBtGatt *_s_CBtGatt;

static const uint8_t _adv_data[] = 
{
    // Flags general discoverable, BR/EDR not supported
    2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,

    // Name
    5, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'Z', 'C', '9', '5', 
   
    // Pulse stream service 
    17, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x00, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xcd, 0xa9, 0xef, 0x11, 0xad, 0x0b, 0xc0, 0x44, 0x77, 0xac,
     
     // Battery service
     3, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_BATTERY_SERVICE & 0xff, ORG_BLUETOOTH_SERVICE_BATTERY_SERVICE >> 8, 
};
static const uint8_t _adv_data_len = sizeof(_adv_data);

CBtGatt::CBtGatt(CRoutineOutput *routine_output, std::vector<CRoutines::Routine> *routines)
{
    _s_CBtGatt = this;
    _routine_output = routine_output;
    _routines = routines;
}

CBtGatt::~CBtGatt()
{
    _s_CBtGatt = NULL;
    _routine_output->stop_routine();
}

void CBtGatt::init()
{
    l2cap_init();
    sm_init();

    // setup ATT server
    att_server_init(profile_data, CBtGatt::s_att_read_callback, CBtGatt::s_att_write_callback);    

    // setup battery service
    battery_service_server_init(25); // TODO: supply bat %

    // need to call battery_service_server_set_battery_value()

    // setup advertisements
    uint16_t adv_int_min = 0x0300;
    uint16_t adv_int_max = 0x0300;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0,sizeof(bd_addr_t));
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(_adv_data_len, (uint8_t*) _adv_data);
    gap_advertisements_enable(1);

    // register for HCI events
    _hci_event_callback_registration.callback = &CBtGatt::s_packet_handler;
    hci_add_event_handler(&_hci_event_callback_registration);

    // register for ATT event
    att_server_register_packet_handler(CBtGatt::s_packet_handler);

    hci_power_control(HCI_POWER_ON);

    _routine_output->activate_routine(find_routine_by_name("DirectPulse"));

}

void CBtGatt::s_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (_s_CBtGatt != NULL)
        _s_CBtGatt->packet_handler(packet_type, channel, packet, size);
}

void CBtGatt::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    hci_con_handle_t con_handle;
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) 
    {
        case HCI_EVENT_META_GAP:
            printf("HCI_EVENT_META_GAP\n");

        case HCI_EVENT_LE_META:
            {
                printf("HCI_EVENT_LE_META\n");

                
            }
            break;
    }

}

uint16_t CBtGatt::s_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
{
    /*
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE)
    {
        return att_read_callback_handle_blob((const uint8_t *)counter_string, counter_string_len, offset, buffer, buffer_size);
    }
    return 0;
    */
   return 0;
}



/**
 * @brief ATT Client Write Callback for Dynamic Data
 * Each Prepared Write Request triggers a callback with transaction mode ATT_TRANSACTION_MODE_ACTIVE.
 * On Execute Write, the callback will be called with ATT_TRANSACTION_MODE_VALIDATE and allows to validate all queued writes and return an application error.
 * If none of the registered callbacks return an error for ATT_TRANSACTION_MODE_VALIDATE and the callback will be called with ATT_TRANSACTION_MODE_EXECUTE.
 * Otherwise, all callbacks will be called with ATT_TRANSACTION_MODE_CANCEL.
 *
 * If the additional validation step is not needed, just return 0 for all callbacks with transaction mode ATT_TRANSACTION_MODE_VALIDATE.
 *
 * @param con_handle of hci le connection
 * @param attribute_handle to be written
 * @param transaction - ATT_TRANSACTION_MODE_NONE for regular writes. For prepared writes: ATT_TRANSACTION_MODE_ACTIVE, ATT_TRANSACTION_MODE_VALIDATE, ATT_TRANSACTION_MODE_EXECUTE, ATT_TRANSACTION_MODE_CANCEL
 * @param offset into the value - used for queued writes and long attributes
 * @param buffer 
 * @param buffer_size
 * @param signature used for signed write commmands
 * @return 0 if write was ok, ATT_ERROR_PREPARE_QUEUE_FULL if no space in queue, ATT_ERROR_INVALID_OFFSET if offset is larger than max buffer
 */
int CBtGatt::s_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    switch (att_handle)
    {

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9A01_01_VALUE_HANDLE: // Pulse stream Characteristic
            //printf("Write: transaction mode %u, offset %u, data (%u bytes): ", transaction_mode, offset, buffer_size);
          //  printf_hexdump(buffer, buffer_size);

            _s_CBtGatt->process_pulse_stream_message(buffer, buffer_size);

            break;
//        case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
//            printf("Write: transaction mode %u, offset %u, data (%u bytes): ", transaction_mode, offset, buffer_size);
//            printf_hexdump(buffer, buffer_size);
//            break;
        default:
            break;

    }
    return 0;
}

// The fist byte of the message is the number of pulses it contains
// There should then be that number of 13 bytes pulses
void CBtGatt::process_pulse_stream_message(uint8_t *buffer, uint16_t buffer_size)
{
    static uint16_t packet_count = 0;
    static uint64_t last_debug_msg = 0;

    if (buffer_size == 0)
        return;

    uint8_t pulse_count = buffer[0];
    uint16_t expected_buffer_size = (sizeof(ble_message_pulse_t) * pulse_count) + 1;

    if (buffer_size != expected_buffer_size)
    {
        printf("process_pulse_stream_message: unexpected message size - %d vs expected %d for %d pulses\n", 
            buffer_size, expected_buffer_size, pulse_count);
        return;
    }
/*        
    packet_count++;
    if (time_us_64() - last_debug_msg > 1000 * 1000)
    {
        printf("pck=%d, ms=%llu\n", packet_count, (time_us_64() - last_debug_msg) / 1000);
        last_debug_msg = time_us_64();
        packet_count = 0;
    }
*/
    for (uint8_t pulse_index = 0; pulse_index < pulse_count; pulse_index++)
    {
        ble_message_pulse_t *msg = (ble_message_pulse_t*)(buffer + 1 + (pulse_index * sizeof(ble_message_pulse_t)));
        /*
        printf("cmd_type         = %d\n"  , msg->cmd_type);
        printf("pulse_width      = %d\n"  , msg->pulse_width);
        printf("amplitude        = %d\n"  , msg->amplitude);
        printf("time_us          = %llu\n", msg->time_us);
        printf("channel_polarity = %d\n"  , msg->channel_polarity);
        */

        switch(msg->cmd_type)
        {
            case BLE_MSG_PULSE_RESET:
                printf("process_pulse_stream_message: START\n");
                _start_time_us = time_us_64();
                break;
            
            case BLE_MSG_PULSE_PULSE:
                {
                    for(uint8_t chan=0; chan < MAX_CHANNELS; chan++)
                    {
                        pulse_message_t pulse_msg = {0};
                        uint8_t chan_opt = pulse_message_for_channel(chan, msg->channel_polarity);
                        if (chan_opt)
                        {
                            if (chan_opt == 0x01 || chan_opt == 0x03)
                            {
                                pulse_msg.pos_pulse_us = msg->pulse_width;
                            }
                            
                            if (chan_opt == 0x02 || chan_opt == 0x03)
                            {
                                pulse_msg.neg_pulse_us = msg->pulse_width;
                            }
                            
                            pulse_msg.abs_time_us = msg->time_us + _start_time_us;
                            
                            if (!queue_try_add(&gPulseQueue[chan], &pulse_msg))
                            {
                                printf("gPulseQueue FIFO was full for chan %d\n", chan);
                            }
                        }
                    }
                }
                break;
        }
    }

    return;
}
// Channel 0-3
uint8_t CBtGatt::pulse_message_for_channel(uint8_t channel, uint8_t channel_polarity)
{
    uint8_t chan_conf = channel_polarity >> (channel * 2);
    chan_conf &= 0x03;

    return chan_conf;
}


int8_t CBtGatt::find_routine_by_name(std::string name)
{
    // TODO
    return 0;
}

