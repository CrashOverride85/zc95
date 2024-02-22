#include "../globals.h"
#include "CBluetooth.h"
#include "btstack.h"

CBluetooth::CBluetooth(CRadio *radio)
{
    printf("CBluetooth()\n");
    _radio = radio;
}

CBluetooth::~CBluetooth()
{
    printf("~CBluetooth()\n");
}

void CBluetooth::pair(bd_addr_t address)
{
    if (_state != state_t::PAIR)
    {
        _cBluetoothPair.set_address(address);
        set_state(state_t::PAIR);
    }
}

void CBluetooth::connect(bd_addr_t address)
{
    if (_state != state_t::PAIR)
    {
        _cBluetoothConnect.set_address(address);
        set_state(state_t::CONNECT);
    }
}

void CBluetooth::set_state(state_t new_state)
{
    if (new_state == _state)
        return;

    if (new_state != state_t::OFF)
        _radio->bluetooth(true);

    // stop old mode
    if (_state == state_t::SCAN)
        _cBluetoothScan.stop();

    if (_state == state_t::PAIR)
        _cBluetoothPair.stop();

    if (_state == state_t::CONNECT)
        _cBluetoothConnect.stop();

    // start new mode
    if (new_state == state_t::SCAN)
        _cBluetoothScan.start();

    if (new_state == state_t::PAIR)
        _cBluetoothPair.start();

    if (new_state == state_t::CONNECT)
    {
        bd_addr_t addr = {0};
        _cBluetoothConnect.get_address(&addr);

        if (is_paired(addr))
        {
            _cBluetoothConnect.start();
        }
        else
        {
            // TODO: error screen
            printf("CBluetooth::set_state(): ERROR - not paired to anything\n");
        }        
    }
    
    // Enable/disable bluetooth
    if (new_state == state_t::OFF)
    {
        hci_power_control(HCI_POWER_OFF);
        _radio->bluetooth(false);
    }
    
    if (new_state != state_t::OFF)
        hci_power_control(HCI_POWER_ON);

    _state = new_state;
}

CBluetooth::state_t CBluetooth::get_state()
{
    return _state;
}

CBluetoothPair::bt_pair_state_t CBluetooth::get_pair_state()
{
    return _cBluetoothPair.get_state();
}

void CBluetooth::scan_get_devices_found(std::vector<CBluetoothScan::bt_device_t>& devices)
{
    _cBluetoothScan.get_devices_found(devices);
}

// Returns true if the address passed in matches that stored in EEPROM, and it's present in the btstack bonding data (flash)
bool CBluetooth::is_paired(bd_addr_t search_address)
{
    bd_addr_t paired_addr = {0};
    g_SavedSettings->get_paired_bt_address(&paired_addr);

    // Does it match the address in EEPROM?
    if (memcmp(search_address, paired_addr, sizeof(bd_addr_t)))
    {
        printf("CBluetooth::is_paired(): false - doesn't match EEPROM\n");
        return false;
    }

    // Does btstack know about it? (not 100% sure this is right)
    // It might be in eeprom but not btstack if flash was cleared but not EEPROM
    for (int idx=0; idx < le_device_db_max_count(); idx++)
    {
        bd_addr_t known_address;
        int address_type = BD_ADDR_TYPE_UNKNOWN;
        
        le_device_db_info(idx, &address_type, known_address, NULL);
        if (address_type == BD_ADDR_TYPE_UNKNOWN)
            continue;
        
        if ((address_type == BD_ADDR_TYPE_LE_PUBLIC) && 
            (memcmp(known_address, search_address, sizeof(bd_addr_t)) == 0))
        {
            return true;
        }
    }
    printf("CBluetooth::is_paired(): false - no match found in btstack flash\n");

    return false;
}
