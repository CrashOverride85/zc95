
# ZC95

*IMPORTANT*: If you downloaded the GERBER files before 2022/02/04, the files for the back/front panel (the one with the POTs on) was wrong... sincere apologies if anyone has ordered this. The "Bad" board would be labeled "ZC95-FrontPanel(back) v0.2", the good one "ZC95-FrontPanel-Analog(back) v0.1".


## Introduction
The ZC95 is a DIY four channel EStim box with similar form factor & output design to the MK312-BT (which in turn is a clone of the ET-312B).
Unlike the 312B, it uses 2x Raspberry Pico microcontrollers instead of an ATMEGA16, and the firmware is open source and mostly written in C++.

Compared to an MK312-BT, it has 2 extra channels, two trigger inputs (think predicament bondage), and an accessory port. It is missing the audio input, bluetooth and, _so far_, most of the patterns of the 312.
Additionally, if a 433MHz transmitter is fitted it can be used to control certain types of shock collars from some patterns.

It's mostly built using through hole parts, with one PCB being mostly SMD - but using parts available through the JLCPCB SMT assembly service. The only exception to this is the ADC for the front panel which is a SOIC-16 footprint so should still be easy enough for most to solder.

### Assembled ZC95
![zc95]


## Overview
The ZC95 consists of 4 PCBs:

* Front panel - no traces, just text / mounting holes
* Front panel controls - 4x POTs and associated ADC, 1x rotary encoder and 6 serial RGB LEDs
* Main board - power supply / charging, MCU for display / pattern generation, button input etc
* Output board ("ZC624 Output module") - 4 channel output generation controlled via SPI from the main board. Designed for production using the SMT assembly service at JLCPCB, with a handful of extra through hole parts also requiring hand soldering.

The primary reason for having separate PCBs for the main and output board is for future flexibility - most of the time & expense is the firmware, case, display, controls, etc., so being able to test new output designs whilst being able to keep all of that should be an advantage. I'm well aware the output design currently used is known to have flaws, mostly stemming from its lack of feedback (which applies equally to the 312b it was taken from). A challenge for another day.

## [Build guide](docs/Build.md)

## [Schematics](schematics/)

## [Gerbers](pcb/)

## [Compiled firmware](https://github.com/CrashOverride85/zc95/releases/)

## [Operation notes](docs/Operation.md)

## [Pattern guide](docs/Patterns.md)

# TODO
## Firmware
   - Pattern to make use of trigger inputs to shock either when triggered, or when not (make configurable)
   - Combo pattern? Having, e.g., waves on channels 1+2 plus something like TENS on 3+4 would be good
   - Two way comms with the the ZC624 output board - e.g. if the ZC624 fails to initialise correctly (or is missing), report this
   - Save pattern settings on exit?

# Hardware
   - Possible audio input - the main board has been designed with an audio input expansion board getting audio via the port labelled serial in mind
   - Maybe a better power switch? although still being able to get all the parts for the main board from LCSC is good
   - Low battery shutoff


[zc95]: docs/images/zc95.jpg "Assembled ZC95"

