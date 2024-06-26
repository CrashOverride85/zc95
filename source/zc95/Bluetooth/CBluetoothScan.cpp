#include "CBluetoothScan.h"
#include "pico/async_context_poll.h"

static CBluetoothScan *_s_cBluetoothScan;

CBluetoothScan::CBluetoothScan()
{
    _s_cBluetoothScan = this;
    _started = false;
}

CBluetoothScan::~CBluetoothScan()
{
    stop();
    _s_cBluetoothScan = NULL;
}

void CBluetoothScan::start()
{
    if (!_started)
    {
        printf("CBluetoothScan::start()\n");
        _started = true;
        _devices_found.clear();

        _hci_event_callback_registration.callback = &CBluetoothScan::s_packet_handler;
        hci_add_event_handler(&_hci_event_callback_registration);

        gap_set_scan_parameters(1,48,48);
        gap_start_scan(); 
    }
}

void CBluetoothScan::stop()
{
    if (_started)
    {
        printf("CBluetoothScan::stop()\n");
        gap_stop_scan();
        hci_remove_event_handler(&_hci_event_callback_registration);
        _started = false;
        _devices_found.clear();
    }
}

void CBluetoothScan::s_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (_s_cBluetoothScan != NULL)
        _s_cBluetoothScan->packet_handler(packet_type, channel, packet, size);
}

void CBluetoothScan::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (packet_type != HCI_EVENT_PACKET) 
        return;

    bd_addr_t address;
    uint8_t length;
    const uint8_t *data;

    switch (hci_event_packet_get_type(packet)) 
    {
        case GAP_EVENT_ADVERTISING_REPORT:
            gap_event_advertising_report_get_address(packet, address);

            length = gap_event_advertising_report_get_data_length(packet);
            data = gap_event_advertising_report_get_data(packet);

            process_advertising_report(data, length, address);
            break;

        default:
            break;
    }
}

void CBluetoothScan::process_advertising_report(const uint8_t *adv_data, uint8_t adv_size, bd_addr_t address)
{
    ad_context_t context;
    bt_device_t scan_entry;
    scan_entry.type = CSavedSettings::bt_device_type_t::NOT_RECEIVED;
    bool got_entry = false;

    for (ad_iterator_init(&context, adv_size, (uint8_t *)adv_data); ad_iterator_has_more(&context); ad_iterator_next(&context))
    {
        uint8_t data_type    = ad_iterator_get_data_type(&context);
        uint8_t size         = ad_iterator_get_data_len(&context);
        const uint8_t *data  = ad_iterator_get_data(&context);

        if (data_type == BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME || data_type == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME)
        {
            std::string name;
            name.assign((const char*)data, size);
            
            if (!address_in_list(address))
            {
                memcpy(scan_entry.address, address, BD_ADDR_LEN);
                scan_entry.name = name;
                got_entry = true;
            }

            break; 
        }

        else if (data_type == BLUETOOTH_DATA_TYPE_APPEARANCE)
        {
            // All shutter remotes I've tested report this as 0x3C1, which is:
            //  category     = 0x00F - HID
            //  sub-category = 0x01  - Keyboard
            uint16_t appearance = little_endian_read_16(data, 0);
            uint16_t category = appearance >> 6;

            switch(category)
            {
                case 0x00F:
                    scan_entry.type = CSavedSettings::bt_device_type_t::HID;
                    break;

                default:
                    scan_entry.type = CSavedSettings::bt_device_type_t::OTHER;
                    printf("Unknown device type, appearance value = 0x%X\n", appearance);
                    break;
            }
        }
    }

    if (got_entry)
    {
        _devices_found.push_back(scan_entry);
        printf("Name: %s (type = %s)\n", scan_entry.name.c_str(), s_get_bt_type_string(scan_entry.type).c_str());
    }
}

std::string CBluetoothScan::s_get_bt_type_string(CSavedSettings::bt_device_type_t type)
{
    std::string type_string = "<UNKNOWN>";
    switch(type)
    {
        case CSavedSettings::bt_device_type_t::HID:
            type_string = "HID";
            break;

        case CSavedSettings::bt_device_type_t::OTHER:
            type_string = "OTHER";
            break;

        case CSavedSettings::bt_device_type_t::NOT_RECEIVED:
            type_string = "<NOT_RECEIVED>";
            break;

        default:
            type_string = "?";
            break;
    }

    return type_string;
}

bool CBluetoothScan::address_in_list(bd_addr_t address)
{
    for (uint8_t n=0; n < _devices_found.size(); n++)
        if (memcmp(_devices_found[n].address, address, BD_ADDR_LEN) == 0)
            return true;

    return false;
}

void CBluetoothScan::get_devices_found(std::vector<CBluetoothScan::bt_device_t>& devices)
{
    devices.clear();

    for (std::vector<CBluetoothScan::bt_device_t>::iterator it = _devices_found.begin(); it != _devices_found.end(); it++)
    {
        devices.push_back((*it));
    }
}
