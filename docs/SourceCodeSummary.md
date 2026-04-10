# ZC95 source

## Summary
* `zc95Bootloader` - bootloader for the zc95. This builds `zc95Bootloader.bin`, which is included by the main zc95 project, so no need to upload the bootloader separately (although it can be).
* `zc95` - main ZC95 source code. Copies in the zc95Bootloader, so that needs to be built first.
* `zc624Bootloader` - Same as `zc95Bootloader`, except for the zc624 (output) pico
* `zc624` - main zc624 source. Copies in the zc624Bootloader, so that needs to be built first.
* `common` - header files that are shared between the above 4 projects. Include hardware config for the zc95 & zc624 which will be common with their respective bootloaders, and i2c message details for communication between zc95 & zc624.

If you're looking to make changes to patterns, the UI etc, the `zc95` source is almost certainly the one you want.


## zc95Bootloader
The bootloader is designed to allow the main firmware (zc95 & zc624) to be updated without having to open the case, by using serial or SD card (a future version of the zc95 firmware should manage downloading to SD).
Updating the bootloader itself still requires a direct USB connection to the Pico; although a benefit of that is that it should be extremely difficult to get the zc95 into a state where it can't be updated via serial.

### Main features
* Accessed using the 3.5mm serial connector, with some debug (output only) on the accessory port
* Menu driven, or API access from the firmware upload tool
* Can backup current firmware (zc95 & zc624) to an SD card, and copy a pending firmware from SD to flash
* An EEPROM flag can be set which triggers the bootloader to update the firmware from SD card
* Checks the ZC624 firmware version is compatible with the zc95 firmware on startup. If it's not, and a compatible version can be found on the SD card, the zc624 is updated to use that.

### Memory map
The image below shows the flash part of the memory map of the zc95, and where the bootloader fits in:

![zc95mem]

When the zc95 firmware is built, instead of including the standard Pico boot2 bootloader, it includes zc95Bootloader (which has boot2 at the start). This means the resulting uf2 file can be copied straight to the Pico, without having to upload the bootloader, then use that to upload the zc95 firmware.

### Firmware upload process
Steps to prepare and upload a firmware to the zc95 without using the python tool, and the general overview of what happening:

#### zc95
1. Build the zc95 firmware as usual

2. Use the generated `zc95.bin` file from the build directory, or use a uf2 to bin converter (what the upload tool does)

3. Remove the first 48k bytes of the file, e.g.: `dd if=./zc95.bin of=./zc95.for_upload.bin bs=1 skip=48k`
  
   This is to strip off the bootloader, which can't be updated via serial.

4. Use a serial terminal which supports XMODEM (minicom is good here), connect to the 3.5mm serial connector on the zc95, set to 115200 8N1

5. Hold down ESC, and power on the box, after a banner showing the version, you should see a menu

6. Pick `[ 1 ] Manage ZC95 firmware`, then `[ 5 ] Upload f/w direct to flash`

7. In the serial terminal, start an XMODEM/CRC transfer of the `zc95.for_upload.bin` file

  After a few moments, the transfer should start. After the first block is received, the bootloader will wipe all flash after the first 48k used for the bootloader, then start writing the received file 256 bytes at a time. The very first 256 bytes of the file is buffered, and only written at the end after the transfer completes; this hopefully ensure partial transfers result in a non-booting box, rather than an unstable box.

  After the transfer is completed, the menu is re-displayed, and `[ 7 ] Launch f/w` can be used to start the new firmware.
  If the firmware is invalid / corrupt, power cycling the box will result in it staying in the bootloader (left and right most LEDs illuminated purple), showing an error on the 3.5mm serial connection, then showing the menu.

  The bootloader can detect obviously bad firmware by looking at the first 128 bytes. This is a version block which includes a version string, compatible zc624 versions, and a magic 32bit value that must be present.
  This is enough to protect against the wrong file being uploaded, but more subtle errors won't be picked up and it'll try to start it.

#### zc624
Essentially follow steps 1-7, but pick the zc624 option from the menu.

The bootloader will then check the zc624 is still in bootloader mode, send a flash erase command over i2c, then start transferring the received firmware in 128 byte blocks as received via XMODEM. Like with the zc95, it will buffer the first 256 byte block and write that last.

## zc624Bootloader
Similar in structure - i.e. same memory map, but there is no way of directly interacting with it. It receives instructions via i2c from the zc95Bootloader, doesn't have access to a SD card and is generally much simpler.


[zc95mem]: images/zc95_mem_map.png "zc95 memory map"
