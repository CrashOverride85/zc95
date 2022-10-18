
# ZC95

## Introduction
The ZC95 is a DIY four channel EStim box with similar form factor & output design to the MK312-BT (which in turn is a clone of the ET-312B).
Unlike the 312B, it uses 2x Raspberry Pico microcontrollers instead of an ATMEGA16, and the firmware is open source and mostly written in C++.

Compared to an MK312-BT, it has 2 extra channels, two trigger inputs (think predicament bondage), and an accessory port. It is missing the audio input, bluetooth and, _so far_, most of the patterns of the 312.
Additionally, if a 433MHz transmitter is fitted it can be used to control certain types of shock collars from some patterns.

It's mostly built using through hole parts, with one PCB being mostly SMD - but using parts available through the JLCPCB SMT assembly service. The only exception to this is the ADC for the front panel which is a SOIC-16 footprint so should still be easy enough for most to solder.

### Assembled ZC95
![zc95]


## Overview
The ZC95 consists of 4-5 PCBs:

* Front panel - no traces, just text / mounting holes
* Front panel controls - 4x POTs and associated ADC, 1x rotary encoder and 6 serial RGB LEDs
* Main board - power supply / charging, MCU for display / pattern generation, button input etc
* Output board ("ZC624 Output module") - 4 channel output generation controlled via SPI from the main board. Designed for production using the SMT assembly service at JLCPCB, with a handful of extra through hole parts also requiring hand soldering.
* (Optional) audio input board - allows the 3.5mm "Aux" port on the front pannel to be used for audio input

The primary reason for having separate PCBs for the main and output board is for future flexibility - most of the time & expense is the firmware, case, display, controls, etc., so being able to test new output designs whilst being able to keep all of that should be an advantage. I'm well aware the output design currently used is known to have flaws, mostly stemming from its lack of feedback (which applies equally to the 312b it was taken from). A challenge for another day.

## [Build guide](docs/Build.md)

## [Schematics](schematics/)

## [Gerbers](pcb/)

## [Compiled firmware](https://github.com/CrashOverride85/zc95/releases/)

## [Operation notes](docs/Operation.md)

## [Pattern guide](docs/Patterns.md)

# TODO
## Firmware
   - Combo pattern? Having, e.g., waves on channels 1+2 plus something like TENS on 3+4 would be good
   - Save pattern settings on exit?

## Hardware
   - Maybe a better power switch? although still being able to get all the parts for the main board from LCSC is good
   - Low battery shutoff

## Long term road map
May do some of this, all of this, or none of this!

   - LUA scripting. Proof of concept completed. The pico's definitely powerful enough. Not sure if there's enough free RAM for scripts of a useful length
   - Switch to Pico W, and allow some kind of remote access. Maybe just to upload LUA scripts
   - Combine main, output and audio boards into one, and probably switch to almost all SMD so it can be assembled by JLCPCB
   - Probably remove "Aux" socket and swap for seperate "Serial" and "Audio" sockets
   - Use a lithium battery pack instead of a 12v SLA
   - Use a USB charger
   - Maybe a smaller, 3D printed case. Probably keeping the same width/weight though

## Known issues
   - Battery gauge is pretty hopeless

[zc95]: docs/images/zc95.jpg "Assembled ZC95"
