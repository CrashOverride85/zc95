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


    // start new mode
    if (new_state == state_t::SCAN)
        _cBluetoothScan.start();

    if (new_state == state_t::PAIR)
        _cBluetoothPair.start();

    
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

void CBluetooth::scan_get_devices_found(std::vector<CBluetoothScan::bt_device_t>& devices)
{
    _cBluetoothScan.get_devices_found(devices);
}
