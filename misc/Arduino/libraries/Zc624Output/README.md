# Arduino control

## Introduction
This is a library to control a bare ZC624 output module/PCB directly, without any of the other PCBs, from an Arduino (tested on ESP32). This means that the Arduino is responsible for generating patterns.

Firstly, it's probably worth pointing out that this use isn't the focus of the project, and there are a few reasons why the zc624 module isn't ideally suited to this use:
* The PCB is a bit of a weird shape, having been designed to fit a specific case
* The connectors for power, data and estim output are placed in a way so that the board can be plugged into another, and are rather awkward to use without a PCB to plug into
* No reverse polarity protection, so be careful(!)
* Requires 5v and 12v

It might also be worth checking out the [WT-312](https://github.com/WendyTeslaburger/WT-312) module as an alternative, as that's designed as building block to use from other projects.

Also worth mentioning that it needs to be driven using 3.3v logic level, so something like an ESP32, Due, etc., is fine, not the older AVR stuff (Uno, Mega, etc.), but probably mostly a non-issue in 2023.

## Connecting

There are 3 connectors:
1. J1 - at the top, nearest the Pico. For 5v, data, and data ground
2. J2 - in the middle. 12v supply and ground
3. J3 - at the bottom, nearest the transformers. Estim output.

### Power supply
The zc624 requires 2 supplies:
- 5v on J1, pin 9, labeled "5v" + ground on pins 5-8 (labelled GND). Used to supply the Pico & Dac. Needs ~100ma at most, probably less.
- 12v on J2, pins 11 & 12, with ground on 1 & 2. Power usage varies enormously depending on output (high pulse widths and higher frequencies needing more), but a 1A polly/resettable fuse on the 12v input is probably a good idea. Average draw is far less than that. Allowable voltage range is 10.5v to 14.5v.

A least one ground from J1 and one from J2 must be connected, and these should be tied together somewhere.

### Data
The zc624 needs to be connected using both I2C and SPI. I2C is used mostly for checking the status of the module, and SPI is used for sending pulse data & output commands.

The SPI interface is on J1, pins 1-4:
* 1 - MOSI
* 2 - MISO
* 3 - SCK
* 4 - CS

And the I2C is on 11 & 12:
* 11 - SCL
* 12 - SDA

All are labeled on the silkscreen.

### Estim output
The generated estim output signals from the module are on J3, with the channels being on these pins:
*  1 &  2 - Channel 1
*  4 &  5 - Channel 2
*  8 &  9 - Channel 3
* 11 & 12 - Channel 4

## Library

After installing the library, there should be an example available: Zc624Demo

This has only been tested on an ESP32 ("ESP32 WROOM-32 38-pin Development Board"), but might work on other Arduino's. 

The demo queries the ZC624 module for version & status, then enables all 4 channels at different frequencies for 4 seconds, before starting a basic "waves" pattern. 

### Modes of operation
There are two slightly different ways the module can be controlled:
1. Set pulse width and frequency, then enable output
2. Send individual pulses

The power is set separately using `set_power()` in both modes. 

Note that `set_power()` can happen out of order to other commands, i.e. if you send a `set_power()` command immediately followed by a pulse, that pulse _may_ be at the previously set power level. Waiting a couple of hundred microseconds from changing the power level to sending a pulse would ensure it's generated at the right level. 
The ZC95 main box changes power level gradually, and in most patterns it only changes when the front panel controls are changed, so avoiding this limitation.

#### 1. Set pulse width and frequency, then enable output
For a given channel, the pulse width can be set between 0-255us for the positive and negative part of the pulse (for symmetric pulses, make them the same), and the frequency can be set between 1hz - 1000hz.
When `On()` for the channel is called, output is generated using those parameters until `Off()` is called. Note that both pulse width and frequency can be changed without first turning off the output.

This approach is used in the example to enable the 4 channels at different frequencies. 

#### 2. Send individual pulses
The `pulse(<positive pulse us>, <negative pulse us>)` command can be used to send an individual pulse with the specified positive/negative pulse widths. When using this mode, the module has to be sent a continuous stream of pulses to generate output; this approach is used in the "waves()" section of the example. 



