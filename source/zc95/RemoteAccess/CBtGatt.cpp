/*
 * ZC95
 * Copyright (C) 2024  CrashOverride85
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

#include "../globals.h"
#include "../gDebugCounters.h"
#include "../git_version.h"

#include "CBtGatt.h"
#include "GattProfile.h"

#include "pico/util/queue.h"

static CBtGatt *_s_CBtGatt;

static const uint8_t _adv_data[] = 
{
    // Flags general discoverable, BR/EDR not supported
    2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,

    // Name
    5, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'Z', 'C', '9', '5', 
   
    // Pulse stream service 
    //17, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x00, 0x9a, 0x0c, 0x20, 0x00, 0x08, 0xcd, 0xa9, 0xef, 0x11, 0xad, 0x0b, 0xc0, 0x44, 0x77, 0xac,

    // General control service
    17, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x00, 0x9b, 0x0c, 0x20, 0x00, 0x08, 0xcd, 0xa9, 0xef, 0x11, 0xad, 0x0b, 0xc0, 0x44, 0x77, 0xac, 
     
     // Battery service
     3, BLUETOOTH_DATA_TYPE_INCOMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, ORG_BLUETOOTH_SERVICE_BATTERY_SERVICE & 0xff, ORG_BLUETOOTH_SERVICE_BATTERY_SERVICE >> 8, 
};
static const uint8_t _adv_data_len = sizeof(_adv_data);

CBtGatt::CBtGatt(CRoutineOutput *routine_output, std::vector<CRoutines::Routine> const &routines, CSavedSettings *saved_settings)
{
    printf("CBtGatt()\n");
    _s_CBtGatt = this;
    _routine_output = routine_output;
    _routines = routines;
    _routine_running = false;
    _saved_settings = saved_settings;

    // get routine config
    _routine_id = find_routine_by_name_and_set_config("DirectControl"); // populates _routine_conf
    memset(_connected_device_address, 0, sizeof(_connected_device_address));
}

CBtGatt::~CBtGatt()
{
    printf("~CBtGatt()\n");
    routine_run(false);

    if (_init_ran)
    {
        gap_advertisements_enable(0);
        hci_remove_event_handler(&_hci_event_callback_registration);

        att_server_deinit();
        sm_deinit();
        l2cap_deinit();
        hci_power_control(HCI_POWER_OFF);
    }

    _s_CBtGatt = NULL;
}

void CBtGatt::init(uint8_t bat_level_pc)
{
    l2cap_init();
    sm_init();

    // setup ATT server
    att_server_init(profile_data, CBtGatt::s_att_read_callback, CBtGatt::s_att_write_callback);    

    // setup battery service
    battery_service_server_init(bat_level_pc);
    _battery_percentage = bat_level_pc;

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
    _init_ran = true;
}

void CBtGatt::loop()
{
    if (time_us_64() - _last_power_status_update_us > (250 * 1000)) // at most every 250ms
    {
        bool update_required = false;
        for(uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
        {
            if (_routine_output->get_front_pannel_power(channel) != _channel_power_change[channel].power_level)
            {
                _channel_power_change[channel].power_level = _routine_output->get_front_pannel_power(channel);
                _channel_power_change[channel].notification_pending = true;
                update_required = true;
            }
        }

        if (update_required)
        {
            att_server_request_can_send_now_event(_bt_connection_handle);
            _last_power_status_update_us = time_us_64();
        }
    }
}

void CBtGatt::s_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (_s_CBtGatt != NULL)
        _s_CBtGatt->packet_handler(packet_type, channel, packet, size);
}

void CBtGatt::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet)) 
    {
        case ATT_EVENT_CONNECTED:
            printf("ATT Connected\n");
            _bt_connection_handle = att_event_connected_get_handle(packet);
            att_event_connected_get_address(packet, _connected_device_address);
            routine_run(true);
            break;

        case ATT_EVENT_DISCONNECTED:
            printf("ATT disconnect\n");
            disconnected();
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            printf("HCI disconnect\n");
            disconnected();
            break;

        case HCI_EVENT_META_GAP:
            // printf("HCI_EVENT_META_GAP\n");
            break;

        case HCI_EVENT_LE_META:
            // printf("HCI_EVENT_LE_META\n");
            break;

        case ATT_EVENT_CAN_SEND_NOW:
            send_notifications();
            break;
    }
}

void CBtGatt::disconnected()
{
    for(uint8_t c = 0; c < MAX_CHANNELS; c++)
    {
        _channel_power_change[c].notifications_enabled = false;
        _channel_power_change[c].notification_pending = false;
    }
    _bt_connection_handle = HCI_CON_HANDLE_INVALID;
    memset(_connected_device_address, 0, sizeof(_connected_device_address));
    routine_run(false);
}

// If the front panel power level dials have changed, and notifications have been requested for power level changes,
// send that notification
void CBtGatt::send_notifications()
{
    if (_bt_connection_handle == HCI_CON_HANDLE_INVALID)
        return;

    for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
    {
        uint16_t attribute_handle = 0;
        if (_channel_power_change[chan].notification_pending && _channel_power_change[chan].notifications_enabled)
        {
            switch(chan)
            {
                case 0:
                    attribute_handle = ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B01_01_VALUE_HANDLE;
                    break;

                case 1:
                    attribute_handle = ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B02_01_VALUE_HANDLE;
                    break;

                case 2:
                    attribute_handle = ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B03_01_VALUE_HANDLE;
                    break;

                case 3:
                    attribute_handle = ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B04_01_VALUE_HANDLE;
                    break;
                
                default:
                    return;
            }

            printf("Send notification for chan %d power level change\n", chan+1);
            att_server_notify(_bt_connection_handle, attribute_handle, (uint8_t*)(&_channel_power_change[chan].power_level), 2);
            _channel_power_change[chan].notification_pending = false;
        }
    }
}

uint16_t CBtGatt::s_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
{
    if (_s_CBtGatt == NULL)
        return 0;

    return _s_CBtGatt->att_read_callback(connection_handle, att_handle, offset, buffer, buffer_size);
}

uint16_t CBtGatt::att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
{   
    switch (att_handle)
    {
        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B01_01_VALUE_HANDLE: // Front panel power setting, channel 1
            return att_read_callback_handle_blob((uint8_t*)(&_channel_power_change[0].power_level), 2, offset, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B02_01_VALUE_HANDLE: // Front panel power setting, channel 2
            return att_read_callback_handle_blob((uint8_t*)(&_channel_power_change[1].power_level), 2, offset, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B03_01_VALUE_HANDLE: // Front panel power setting, channel 3
            return att_read_callback_handle_blob((uint8_t*)(&_channel_power_change[2].power_level), 2, offset, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B04_01_VALUE_HANDLE: // Front panel power setting, channel 4
            return att_read_callback_handle_blob((uint8_t*)(&_channel_power_change[3].power_level), 2, offset, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9A02_01_VALUE_HANDLE: // Channel isolation - allow disable (read only)
            return att_read_callback_handle_blob(allow_channel_isolation_disable(), 1, offset, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9A03_01_VALUE_HANDLE: // Channel isolation - current setting (read/write)
            return att_read_callback_handle_blob((uint8_t*)&gZc624ChannelIsolationEnabled, 1, offset, buffer, buffer_size);            

        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_FIRMWARE_REVISION_STRING_01_VALUE_HANDLE:
            return att_read_callback_handle_blob((uint8_t*)(kGitHash), strlen(kGitHash), offset, buffer, buffer_size);
    }

   return 0;
}

int CBtGatt::s_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    if (_s_CBtGatt == NULL)
        return 0;

    return _s_CBtGatt->att_write_callback(connection_handle, att_handle, transaction_mode, offset, buffer, buffer_size);
}

int CBtGatt::att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    bool notification_enable = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
    
    switch (att_handle)
    {
        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9A01_01_VALUE_HANDLE: // Pulse stream Characteristic
            return process_pulse_stream_packet(buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9A03_01_VALUE_HANDLE: // Channel isolation (read/write)
            return process_set_channel_isolation_packet(buffer, buffer_size);

        // Handle requests to enable/disable notification of front panel power changes
        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B01_01_CLIENT_CONFIGURATION_HANDLE: // Chanel 1 write notification enable
            printf("notification_enable chan1: %d\n", notification_enable);
            _channel_power_change[0].notifications_enabled = notification_enable;
            break;

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B02_01_CLIENT_CONFIGURATION_HANDLE: // Chanel 2 write notification enable
            printf("notification_enable chan2: %d\n", notification_enable);
            _channel_power_change[1].notifications_enabled = notification_enable;
            break;

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B03_01_CLIENT_CONFIGURATION_HANDLE: // Chanel 3 write notification enable
            printf("notification_enable chan3: %d\n", notification_enable);
            _channel_power_change[2].notifications_enabled = notification_enable;
            break;

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B04_01_CLIENT_CONFIGURATION_HANDLE: // Chanel 4 write notification enable
            printf("notification_enable chan4: %d\n", notification_enable);
            _channel_power_change[3].notifications_enabled = notification_enable;
            break;

        /////// Alternative to pulse mode - just set freq, pulse width & power ///////
        
        // Set frequency
        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B21_01_VALUE_HANDLE: 
            return set_channel_frequency(0, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B22_01_VALUE_HANDLE: 
            return set_channel_frequency(1, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B23_01_VALUE_HANDLE: 
            return set_channel_frequency(2, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B24_01_VALUE_HANDLE: 
            return set_channel_frequency(3, buffer, buffer_size);

        // Set pulse width
        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B11_01_VALUE_HANDLE:
            return set_channel_pulse_width(0, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B12_01_VALUE_HANDLE:
            return set_channel_pulse_width(1, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B13_01_VALUE_HANDLE:
            return set_channel_pulse_width(2, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B14_01_VALUE_HANDLE:
            return set_channel_pulse_width(3, buffer, buffer_size);

        // Set power level
        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B31_01_VALUE_HANDLE:
            return set_channel_power_level(0, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B32_01_VALUE_HANDLE:
            return set_channel_power_level(1, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B33_01_VALUE_HANDLE:
            return set_channel_power_level(2, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B34_01_VALUE_HANDLE:
            return set_channel_power_level(3, buffer, buffer_size);

        // Power enable/disable
        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B41_01_VALUE_HANDLE:
            return set_channel_power_enable(0, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B42_01_VALUE_HANDLE:
            return set_channel_power_enable(1, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B43_01_VALUE_HANDLE:
            return set_channel_power_enable(2, buffer, buffer_size);

        case ATT_CHARACTERISTIC_AC7744C0_0BAD_11EF_A9CD_0800200C9B44_01_VALUE_HANDLE:
            return set_channel_power_enable(3, buffer, buffer_size);


        default:
            printf("Write: att_handle: 0x%x, transaction mode %u, offset %u, data (%u bytes)\n", att_handle, transaction_mode, offset, buffer_size);
            // printf_hexdump(buffer, buffer_size);
            break;

    }
    return 0;
}

// Set frequency for channel (0-3) using min/max menus of running DirectPulse pattern
// menu_ids 30-33 are for setting the frequency of channels 0-3
uint8_t CBtGatt::set_channel_frequency(uint8_t channel, uint8_t *buffer, uint16_t buffer_size)
{
    if (buffer_size != 1)
    {
        printf("Invalid freq set message for chan %d (buffer size = %d vs expected 1)\n", channel, buffer_size);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    uint8_t freq = *buffer;
    printf("set freq for chan %d to %d\n", channel, freq);

    _routine_output->menu_min_max_change(channel+30, freq);

    return 0;
}

// Expecting two bytes - 1=pos pulse width, 2=neg pulse width
uint8_t CBtGatt::set_channel_pulse_width(uint8_t channel, uint8_t *buffer, uint16_t buffer_size)
{
    if (buffer_size != 2)
    {
        printf("Invalid pulse width set message for chan %d (buffer size = %d vs expected 2)\n", channel, buffer_size);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    uint8_t pos_pulse_width = *(buffer);
    uint8_t neg_pulse_width = *(buffer+1);
    printf("set pulse width for chan %d to %d/%d\n", channel, pos_pulse_width, neg_pulse_width);

    _routine_output->menu_min_max_change(channel+10, pos_pulse_width);
    _routine_output->menu_min_max_change(channel+20, neg_pulse_width);
    return 0;
}

// Expecting two bytes for a 16bit value
uint8_t CBtGatt::set_channel_power_level(uint8_t channel, uint8_t *buffer, uint16_t buffer_size)
{
    if (buffer_size != 2)
    {
        printf("Invalid power level set message for chan %d (buffer size = %d vs expected 2)\n", channel, buffer_size);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    uint16_t power_level =  (buffer[0] << 8) | buffer[1];

    if (power_level > 1000)
    {
        printf("Invalid channel power message: %d (max 1000)\n", power_level);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    if (_saved_settings->get_ble_remote_access_power_dial_mode() == CSavedSettings::ble_power_dial_mode_t::LIMIT)
    {
        _routine_output->set_remote_power(channel, power_level);
    }
    else if (_saved_settings->get_ble_remote_access_power_dial_mode() == CSavedSettings::ble_power_dial_mode_t::SCALE)
    {
        _routine_output->menu_min_max_change(channel, power_level);        
    }

    return 0;
}

uint8_t CBtGatt::set_channel_power_enable(uint8_t channel, uint8_t *buffer, uint16_t buffer_size)
{
    if (buffer_size != 1)
    {
        printf("Invalid power enable message for chan %d (buffer size = %d vs expected 1)\n", channel, buffer_size);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    uint8_t enable = buffer[0];

    if (enable > 1)
    {
        printf("Invalid channel power enable message: %d (expected 0 or 1)\n", enable);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    _routine_output->menu_multi_choice_change(40+channel, enable);

    return 0;
}

// The fist byte of the message is the number of pulses it contains
// The second byte is an incrementing 8bit packet counter
// There should then be the a series of 13 byte pulse messages (as specified in the first byte)
uint8_t CBtGatt::process_pulse_stream_packet(uint8_t *buffer, uint16_t buffer_size)
{
    static uint64_t s_last_debug_msg = 0;
    static uint32_t s_recv_packets = 0;
    static uint32_t s_recv_messages = 0;
    static uint8_t s_last_packet_count = 0;
    const uint8_t header_size = 2; // pulse_count & packet_counter are always sent, followed by a variable (pulse_count) number of 13 bytes messages

    if (buffer_size == 0)
        return ATT_ERROR_VALUE_NOT_ALLOWED;

    s_recv_packets++;

    uint8_t pulse_count = buffer[0];
    uint8_t packet_counter = buffer[1];
    uint16_t expected_buffer_size = (sizeof(ble_message_pulse_t) * pulse_count) + header_size;

    if (buffer_size != expected_buffer_size)
    {
        printf("process_pulse_stream_message: unexpected packet size - %d vs expected %d for %d messages\n", 
            buffer_size, expected_buffer_size, pulse_count);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    // Keep track of missed packets, assumes that the packet_counter being sent is incremented by 1 
    // for each packet, wrapping around to zero after 255. Being careful to to stick to uint8_t 
    // types when calculating so warp around works as desired.
    uint8_t expected_packet_count = s_last_packet_count + 1;
    uint8_t missed_packets = packet_counter - expected_packet_count;
    _dbg_missing_packet_counter += missed_packets;
    s_last_packet_count = packet_counter;
       
    if (time_us_64() - s_last_debug_msg > 1000 * 1000)
    {
        printf("Packet count: %lu, Message count: %lu, Missing packets: %lu, fifo full: %lu, msg_old: %lu, msg_past: %lu, msg_future: %lu\n", 
            s_recv_packets,
            s_recv_messages,
            _dbg_missing_packet_counter,
            _dbg_fifo_full_counter, 
            debug_counters_get(dbg_counter_t::DBG_COUNTER_MSG_OLD),
            debug_counters_get(dbg_counter_t::DBG_COUNTER_MSG_PAST),
            debug_counters_get(dbg_counter_t::DBG_COUNTER_MSG_FUTURE)
        );
        debug_counters_reset();

        s_last_debug_msg = time_us_64();
        _dbg_missing_packet_counter = 0;
        _dbg_fifo_full_counter = 0;
        s_recv_packets = 0;
        s_recv_messages = 0;
    }

    for (uint8_t pulse_index = 0; pulse_index < pulse_count; pulse_index++)
    {
        s_recv_messages++;
        ble_message_pulse_t *msg = (ble_message_pulse_t*)(buffer + header_size + (pulse_index * sizeof(ble_message_pulse_t)));

        switch(msg->cmd_type)
        {
            case BLE_MSG_PULSE_RESET:
                printf("process_pulse_stream_message: START\n");
                _start_time_us = time_us_64();
                _dbg_missing_packet_counter = 0;
                _dbg_fifo_full_counter = 0;
                s_last_packet_count = packet_counter;
                debug_counters_reset();
                break;
            
            case BLE_MSG_PULSE_PULSE:
                process_pulse_message(msg);
                break;
        }
    }

    return 0;
}

void CBtGatt::process_pulse_message(ble_message_pulse_t *msg)
{
    if ((msg->time_us + _start_time_us) < time_us_64())
    {
        // Discard pulse messages where the time is in the past
        debug_counters_increment(dbg_counter_t::DBG_COUNTER_MSG_PAST);
        return;
    }

    if (msg->amplitude > 1000)
    {
        debug_counters_increment(dbg_counter_t::DBG_COUNTER_MSG_INVALID);
        return;
    }

    for(uint8_t chan=0; chan < MAX_CHANNELS; chan++)
    {
        pulse_message_t pulse_msg = {0};
        pulse_msg.power_level = msg->amplitude;

        uint8_t chan_opt = pulse_message_for_channel(chan, msg->channel_polarity);
        if (chan_opt)
        {
            if (chan_opt & 0x01)
            {
                pulse_msg.pos_pulse_us = msg->pulse_width;
            }
            
            if (chan_opt & 0x02)
            {
                pulse_msg.neg_pulse_us = msg->pulse_width;
            }
            
            pulse_msg.abs_time_us = msg->time_us + _start_time_us;
            if (!queue_try_add(&gPulseQueue[chan], &pulse_msg))
            {
                _dbg_fifo_full_counter++;
            }
        }
    }
}

uint8_t CBtGatt::process_set_channel_isolation_packet(uint8_t *buffer, uint16_t buffer_size)
{
    if (buffer_size != 1)
    {
        printf("process_set_channel_isolation_packet: unexpected buffer size of %d vs expected 1\n", buffer_size);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    if (buffer[0] != 0 && buffer[0] != 1)
    {
        printf("process_set_channel_isolation_packet: unexpected value received of [%d]. Expected 0 or 1\n", buffer[0]);
        return ATT_ERROR_VALUE_NOT_ALLOWED;
    }

    uint8_t* allow_disable = allow_channel_isolation_disable();
    if (*allow_disable)
    {
        _routine_output->menu_multi_choice_change(100, buffer[0]); // 0=disable channel isolation, 1=enable channel isolation
        return 0;
    }
    else
    {
        printf("process_set_channel_isolation_packet: Rejecting attempt to disable channel isolation when not permitted by config\n");
        return ATT_ERROR_WRITE_NOT_PERMITTED;
    }
}

// Channel 0-3
// When given the channel_polarity section of a message and requested channel number, return channel options:
//  0 = no pulse
//  1 = positive pulse only
//  2 = negative pulse only
//  3 = both
uint8_t CBtGatt::pulse_message_for_channel(uint8_t channel, uint8_t channel_polarity)
{
    uint8_t chan_conf = channel_polarity >> (channel * 2);
    chan_conf &= 0x03;

    return chan_conf;
}

void CBtGatt::set_battery_percentage(uint8_t bat)
{
    if (_battery_percentage != bat)
    {
        battery_service_server_set_battery_value(bat);
        _battery_percentage = bat;
    }
}

void CBtGatt::routine_run(bool run)
{
    if (_routine_running == run)
        return;

    if (run)
    {
        _routine_output->activate_routine(_routine_id);

       if (_saved_settings->get_ble_remote_access_power_dial_mode() == CSavedSettings::ble_power_dial_mode_t::LIMIT)
        {
            // This tells the DirectControl routine to set its output to 100%. In LIMIT mode, received BLE power level commands
            // will be used to set the power level by calling set_remote_power, which is displayed as the blue bar. The routine
            // power level is the inner yellow bar, so in this mode (and like most inbuilt built routes outside of remote access),
            // the yellow bar will always match the blue bar.
            for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
                _routine_output->menu_min_max_change(channel, 1000);
        }
    }
    else
    {
        _routine_output->stop_routine();

        if (_saved_settings->get_ble_remote_access_power_dial_mode() == CSavedSettings::ble_power_dial_mode_t::LIMIT)
        {
            for (uint8_t channel = 0; channel < MAX_CHANNELS; channel++)
                _routine_output->set_remote_power(channel, 0);
        }
    }
    _routine_running = run;
}

int8_t CBtGatt::find_routine_by_name_and_set_config(std::string name)
{
    for (uint8_t idx=0; idx < _routines.size(); idx++)
    {
        CRoutines::Routine routine = _routines[idx];
        CRoutine* routine_ptr = routine.routine_maker(routine.param);
        routine_ptr->get_config(&_routine_conf); // Note _routine_conf class variable
        delete routine_ptr;

        if (std::string(_routine_conf.name) == name)
        {
            _routine_id = idx;
            return _routine_id;
        }   
    }

    printf("find_routine_by_name_and_set_config: ERROR: [%s] not found - things will not work well!\n", name.c_str());
    return 0;
}

// Channel isolation can be disabled if the routine has it's force_channel_isolation option set to false (that should 
// always be the case here) and the BLE config option "Allow triphase" is set to "Yes".
uint8_t* CBtGatt::allow_channel_isolation_disable()
{
    static uint8_t val = true;
    val = !_routine_conf.force_channel_isolation;
    val &= _saved_settings->get_ble_remote_disable_channel_isolation_permitted();

   return &val;
}

bool CBtGatt::is_connected()
{
    return (_bt_connection_handle != HCI_CON_HANDLE_INVALID);
}

const char *CBtGatt::get_connected_device_address()
{
    if (is_connected())
        return bd_addr_to_str(_connected_device_address);
    else
        return "";
}