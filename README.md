
# ZC95

**Note**: This repo has submodules, so if cloning, it's best to use the recursive option.

## Introduction
The ZC95 is a DIY four channel EStim box with similar form factor & output design to the MK312-BT (which in turn is a clone of the ET-312B).
Unlike the 312B, it uses 2x Raspberry Pico microcontrollers instead of an ATMEGA16, and the firmware is open source and mostly written in C++.

The box can be controlled remotely via a Python GUI, and run Lua scripts uploaded to it, either using RS232 serial or WiFi if a Pico-W is used for the main MCU.

Compared to an MK312-BT, it has 2 extra channels, two trigger inputs (think predicament bondage), and an accessory port. It has limited [bluetooth support](docs/Bluetooth.md) (remotes only) and, _so far_, is missing most of the patterns of the 312. Audio input is possible with an extra/optional board.

Additionally, if a 433MHz transmitter is fitted it can be used to control certain types of shock collars from some patterns.

The main board is built using through hole parts, with the output and front panel PCBs being mostly SMD (requiring hand-soldering of a few through hole parts) - but using parts available through the JLCPCB SMT assembly service.

### Assembled ZC95
![zc95]


## Overview
The ZC95 consists of 4-5 PCBs:

* Front panel - no traces, just text / mounting holes
* Front panel controls - 4x POTs and associated ADC, 1x rotary encoder and 6 serial RGB LEDs. Designed for production using the SMT assembly service at JLCPCB, with a handful of extra through hole parts also requiring hand soldering.
* Main board - power supply / charging, MCU for display / pattern generation, button input etc
* Output board ("ZC624 Output module") - 4 channel output generation controlled via SPI from the main board. Designed for production using the SMT assembly service at JLCPCB, with a handful of extra through hole parts also requiring hand soldering.
* (Optional) audio input board - allows the 3.5mm "Aux" port on the front panel to be used for audio input

The primary reason for having separate PCBs for the main and output board is for future flexibility - most of the time & expense is the firmware, case, display, controls, etc., so being able to test new output designs whilst being able to keep all of that should be an advantage. I'm well aware the output design currently used is known to have flaws, mostly stemming from its lack of feedback (which applies equally to the 312b it was taken from). A challenge for another day.

The zc624 output module can also be used standalone and [controlled from an Arduino](./misc/Arduino/libraries/Zc624Output/README.md) (tested with an ESP32), but this is not ideal for a few reasons, and not the focus of this project.

## [Build guide](docs/Build.md)

## [Schematics](schematics/)

## [Gerbers](pcb/)

## [Compiled firmware](https://github.com/CrashOverride85/zc95/releases/)

## [Operation notes](docs/Operation.md)

## [Pattern guide](docs/Patterns.md)

## [Lua scripting notes](docs/LuaNotes.md)

## [Notes on building source](docs/SourceBuildNotes.md)

## [Credits](docs/Credits.md)

# TODO
## Firmware
   - Combo pattern? Having, e.g., waves on channels 1+2 plus something like TENS on 3+4 would be good
   - Save pattern settings on exit?

## Hardware
   - Maybe a better power switch? although still being able to get all the parts for the main board from LCSC is good
   - Low battery shutoff

## Long term road map
May do some of this, all of this, or none of this!
   - Combine main, output and audio boards into one, and probably switch to almost all SMD so it can be assembled by JLCPCB
   - Probably remove "Aux" socket and swap for separate "Serial" and "Audio" sockets
   - Use a lithium battery pack instead of a 12v SLA
   - Use a USB charger
   - Maybe a smaller, 3D printed case. Probably keeping the same width/weight though

## Known issues
   - Can hang on entering WiFi setup (AP) mode due to bad choice of MOSFET Q1 (see [#25][gh25] & [#46][gh46]). 
     
     Fix:
     * Fit a Schottky diode (e.g. 1N5819) in place of Q1 - see build notes

     Workarounds:
     * Power the Pico over USB whilst entering AP mode (power box on first, then connect USB)
     * Directly connect VSYS on the Pico to 5v. Do not connect the box to USB whilst switched on if doing this.
   
   - Battery gauge is pretty hopeless
   - Depending on the pattern, the LEDs aren't very useful - either Red or Green, with no dimming depending on power

[zc95]: docs/images/zc95.jpg "Assembled ZC95"
[gh25]: https://github.com/CrashOverride85/zc95/discussions/25
[gh46]: https://github.com/CrashOverride85/zc95/issues/46
