# Bluetooth HID example

## Summary
This is the counterpart to the example [bluetooth_hid.lua](../../../remote_access/lua/bluetooth_hid.lua) script. It requires a separate Pico-W & a potentiometer. As the potentiometer is changed, the power level on the connected ZC95 running the bluetooth_hid script is changed.
It's intended as an example of the bluetooth support, not a useful project in its own right.

## Build
### Hardware
Connect a potentiometer (anything between 10k & 100k should be fine) between a ground pin and the 3v3 out pin of the Pico, and connect the wiper to pin 31 (ADC0/GP26). If needed, serial debug output is on pin 1 (GP0).
Power over USB.

### Firmware
Same as the [main ZC95 firmware](../../../docs/SourceBuildNotes.md) - run cmake & make, and copy the resulting UF2 file to the pico.

## Use
* Upload the [bluetooth_hid.lua](../../../remote_access/lua/bluetooth_hid.lua) script to the ZC95
* Enable bluetooth
* Power on the pico-w with the hid_example firmware
* On the ZC95, go to Config -> Bluetooth -> Scan/Pair
* Hopefully the "Zc Hid Ex" device shows up - pair with it
* Go back, and start the "BtHid" pattern previously uploaded
* Make sure the power is turned up on at least one chanel 

After a few moments it should connect, and turning the potentiometer should change the power level for all 4 channels.
