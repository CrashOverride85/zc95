# Bluetooth support

The Bluetooth (BLE only) support in the ZC95 is split into two types:
1. Bluetooth peripherals - supports connecting to bluetooth remotes  
2. Remote control via Bluetooth - for generating patterns remotely and streaming to the box. 

## Bluetooth peripherals
In this mode, the ZC95 can be used to scan for and connect to a BLE HID device - expected to be BLE shutter remotes. Patterns / Lua scripts still run as normal on the box, and the remote can be used as an input. 

See [Bluetooth Peripherals](./BluetoothPeripherals.md) for more information.

## Bluetooth remote control
In this mode, the ZC95 can be connected to, and patterns generated remotely and streamed to it. This mode is currently aimed at developers only. 

See [BleStreaming](./BleStreaming.md) for more information.
