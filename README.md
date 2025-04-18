
# ZC95

**Note**: This repo has submodules, so if cloning, it's best to use the recursive option.

## ZC95 MkII

This branch holds the ZC95 MkII. As I write this, I'm the only person to have built it. It's smaller, and condenses 3 of the PCBs into 1 to make the build a bit cheaper and much quicker than the Mk1.

These are the main changes from the MkI:
 - Much smaller - the mainboard is less than half the size
 - Output, Audio and main board are now combined
 - USB-C charger input instead of 15v barrel jack
 - 26650 cell instead of 12v SLA battery (similar capacity)
 - "Aux" socket swapped for separate "Serial" and "Audio" sockets
 - Output sockets changed from 2.5mm to 3.5mm
 - Serial selectable (via jumpers) between 3.3v TTL and RS232
 - Display no longer briefly flashes white on power on
 - Removed: 12v output on accessory port

Hopefully at some point this branch will become the main branch, and I'll remove this section.

## Introduction
The ZC95 is a DIY four channel EStim box with similar output design and feel to the MK312-BT (which in turn is a clone of the ET-312B).

![zc95]

The box is primarily intended for standalone use (i.e. not used whilst connected to phone/laptop/tablet), but can be controlled remotely via a Python GUI, and run Lua scripts that have been uploaded to it over serial or WiFi.

Main features:
- 4 isolated channels 
- [Audio input](./docs/AudioInput-Operation.md) - line level for estim tracks or microphone input (selectable via menu)
- [Many inbuilt patterns](./docs/Patterns.md) - around 14, not counting audio and more esoteric stuff
- Custom patterns can be written in [Lua](./docs/LuaNotes.md)
- Two stereo 3.5mm trigger input sockets, allowing for up to 4 external inputs (e.g. remote buttons, foot pedals, etc.)
- Support for many [BLE remote/camera buttons](./docs//BluetoothPeripherals.md), e.g. to trigger output when the button is pressed
- [Remote control](./docs/RemoteAccess.md) over WiFi or serial using a python app
- Allows limited control of a certain type of 433MHz shock collar
- Somewhat experimental [BLE steaming](./docs/BleStreaming.md) - allows for per-pulse level control from laptop or other device, with an example in python
- An accessory port with 3 output lines that can be controlled from Lua scripts, to allow interacting with other devices

Technical details:
- Has been designed with the JLC PCB assembly service in mind, and is mostly an SMD design with a few though hole parts
- Uses 2x Raspberry Picos
- Firmware is open source and mostly C++, and makes use of many [3rd party libraries](./docs/Credits.md)
- 22650 cell for power
- KiCad project files included in repo, not just gerbers
- Audio input includes microphone preamp that can be toggled via menu, along with variable gain
- Consists of 3 PCBs:
   * Front panel - no traces, just text / mounting holes
   * Front panel controls - 4x POTs and associated ADC, 1x rotary encoder and 6 serial RGB LEDs. Designed for production using the SMT assembly service at JLCPCB, with a handful of extra through hole parts also requiring hand soldering.
   * Main board - power supply / charging, pattern generation, button input, output signal generation, audio input
- Case is either an off the shelf Hammond 1598DBK (larger), or a 3D printed case (step/stl files provided)

## [Build guide](docs/Build.md)

## [Schematics](schematics/)

## [Gerbers](pcb/)

## [Compiled firmware](https://github.com/CrashOverride85/zc95/releases/)

## [Operation notes](docs/Operation.md)

## [Pattern guide](docs/Patterns.md)

## [Lua scripting notes](docs/LuaNotes.md)

## [Notes on building source](docs/SourceBuildNotes.md)

## [Credits](docs/Credits.md)

# Miscellaneous  
## History
The ZC95 is very heavily inspired by the MK312-BT, which is a reversed engineered version of the ET-312. The original repo for that project is long gone, but there is a fork [here](https://github.com/CrashOverride85/mk312-bt).

The ZC95 mk1 was made public in late 2021, and had a similar form factor to the MK312. That version has been built successfully by many.

The mk1 zc95 had the output stage on a separate PCB. It's no longer part of the zc95 build, but could still be built and used to add 4 channel estim output to an Arduino (with notes for & tested with an ESP32).
The PCB / BoM / build notes for it are [here](./misc/OutputModule/).

## Support, feedback, etc.
The ZC95 project is provided 'as is' without warranty of any kind, either express or implied. The project is provided for your use at your own risk.

Having said that, if you run into difficulties or have questions, the two best places to try are:
- Github [discussions](https://github.com/CrashOverride85/zc95/discussions) or [issues](https://github.com/CrashOverride85/zc95/issues)
- The ZC95 channel on [Joanne's E-Stim Community discord](https://discord.gg/HbGKY2t), see Estim control boxes -> zc95
  - I'd highly recommend joining this discord server anyway, the zc95 chanel is a _tiny_ part of what's there


## Other projects
Other related projects that are worth checking out:

- [NeoDK](https://github.com/Onwrikbaar/NeoDK) - Advanced electrostimulation machine development kit. In active development, and also has a chanel on [Joanne's E-Stim Community discord](https://discord.gg/HbGKY2t)

- MK-312BT - Project the ZC95 was inspired by, and shares lot in common with (original repo gone, but there are many [forks](https://github.com/CrashOverride85/mk312-bt))

- [WT-312](https://github.com/WendyTeslaburger/WT-312) - 312 style driver, intended for integration into other projects

- [FOC-Stim](https://github.com/diglet48/FOC-Stim) - In development, checkout the foc-stim channel on [Joanne's E-Stim Community discord](https://discord.gg/HbGKY2t) for the latest


# TODO
## Firmware
   - Combo pattern? Having, e.g., waves on channels 1+2 plus something like TENS on 3+4 would be good
   - Save pattern settings on exit?
   - Disable when plugged in / switch to charging screen (maybe a setting in the menu to disable)
   - Allow Lua scripts to interact with shock collars
   - Accessory port doesn't do much - just 3 output lines. There's a serial interface there too, which could be exposed in the Lua environment

## Hardware
   - More suitable charge controller instead of TP4056
   - Improved 3D printed case?

## Known issues
   - Excessive capacitance on USB power input. Doesn't seem to cause a problem with USB chargers, and the USB socket is for charging only.
   - When plugged in, all power is drawn from the USB input. Things can go wrong if it tries to draw more power than the charger can supply, which is especially likely if using the box whilst it's also charging,  A future firmware version might make the box change-only when plugged in. 
   - Both issues could likely be addressed by swapping to more suitable charge controller for this application (e.g. BQ24075)
   - A bit slow to charge; it currently charges at ~780mA, which for a 5300mAh cell means a charge time from 0% to 100% of around 7 hours

[zc95]: docs/images/powered_up.jpg "Assembled ZC95"
[gh25]: https://github.com/CrashOverride85/zc95/discussions/25
[gh46]: https://github.com/CrashOverride85/zc95/issues/46
