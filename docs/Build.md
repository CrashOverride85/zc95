# Hardware build notes 

## Prerequisites 
These notes assume a reasonable amount of experience assembling electronic kits. I suggest having a read though these notes before ordering anything.

Not covered, but assumes you have a crimp tool for molex kk style connectors, something to make up IDC cables (I just use a mini-vice) and the usual tools.

## PCB + Parts ordering

The ZC95 is made up of 3 PCBs:
1. Main board
2. Front panel controls - board with potentiometers, LEDs etc on it
3. Front panel - no components, previous board is mounted to the back of it

1 + 2 have CPL & BOM files as well as gerbers, and are aimed at JLCPCB SMT assembly service. 

For the pair of front panel PCBs, there are two options:
1. Original version (v0.2): through hole LEDs, hand soldered (round) buttons with leads attaching to a smaller PCB
  
  ![zc95 powered up]

2. New version (v1.1), kindly contributed by rootuz: SMD LEDs with light pipes to front, (square) buttons mounted directly onto a larger PCB

  ![fp v1.1]

For either option, order a matching pair of "Front panel controls" and "Front panel" boards.

### PCBs

#### 1. Main board
Order from JLCPCB using the linked gerbers and default settings (FR-4, 1.6mm thick, 1oz, etc.):

* [Main board](../pcb/MainBoard/GERBER-main.zip)

Select the "SMT Assembly" option at the bottom - pick "Assemble top side" and Tooling holes "Added by JLCPCB". 
On the next page, add the BOM and CPL files:

* [BOM-main.csv](../pcb/MainBoard/BOM-main.csv)
* [CPL-main.csv](../pcb/MainBoard/CPL-main.csv)

#### 2. Front panel
Order a pair of matching front panel PCBs - see "Order PCBs" section in relevant notes:
* [Notes for v0.2 - round buttons](./Build_fp_0_2.md)
* [Notes for v1.1 - square buttons](./Build_fp_1_x.md)

### PCB Parts
All remaining parts not covered by the JLC PCB assembly service required to populate the PCBs - with the exception of the transformers - can be purchased from LCSC, and this BOM spreadsheet lists required parts + quantity with the LCSC part number for each board on separate tabs:

[BoM (openoffice format)](BoM.fods)

#### Part substitutions
A few parts in particular are likely to cause problems if substituted:

* Transformers. The zc95 is designed around the 42TL004, and that's what all my testing (Mk1 & Mk2) has been with. The larger 42TU200's would _likely_ also work, but I'm not sure if they'll physically fit - there is space / mounting holes on the main PCB for them, but I suspect there wouldn't be enough clearance between them and the front panel. Expect problems using other transformers unless they are a very close match.

* IRF9Z24NPBF P-chanel MOSFETs - if out of stock with JLC, I strongly advise either obtaining these elsewhere and fitting yourself, or waiting. The design is _very_ sensitive to any changes to this particular part. 


### Case
There are two options for a case:
1. Hammond 1598DBK - same case as Mk1
2. 3D printed case - works, but somewhat of a work in progress

The Hammond case is just over twice as deep as the 3D printed case, but if size isn't a concern, it's a good option as it's strong/well made, readily available and quite affordable.

#### 3D printed case
The case is based on a design for the Mk1 case by [@liselotte111](https://github.com/CrashOverride85/zc95/discussions/84), that I've modified for the Mk2.
I've not got a 3D printer, and editing the case to fit the Mk2 is my first attempt at doing something like that, so beware! 

There are three parts:

1. [Top section](../misc/Mk2Case/top.step) STEP
2. [Bottom section](../misc/Mk2Case/bottom.step) STEP
3. [End plate](../misc/Mk2Case/endplate.stl) STL

I got mine produced by JLCPCB, and went for the "FDM(Plastic)", "ABS", black option. No idea if that's the most suitable, but the result seemed ok.

### Misc parts
[The BoM spreadsheet](BoM.fods) includes a Misc tab with the rest of the parts required to complete the build (battery, display, etc.).

* Battery: Use a **protected** 26650 cell that can can be safely charged at 2000 mA. The firmware currently has a hardcoded assumption of a 5300mAh cell, but it will learn the battery to an extent after a full discharge/charge cycle, so anything reasonably close should be fine (and it only affects the battery gauge anyway). Be aware that a battery is _required_ for the ZC95 to function correctly. It might power on and pass the self test without it if you're lucky, but expect stability problems without it.

* Display: I would advise sticking to the ADA358 despite the cost, as the front panel has been designed for it. However I'm aware of at least one person who used a generic 1.8" ST7735 display from aliexpress, and it mostly worked ok. 

## Assembly

### Case
Applies to the 3D printed case; there's nothing extra to do for the Hammond one.

If you're familiar with 3D printing stuff, you probably have a better idea than me here. But what I've done:

- The bottom part of the case has two rows of 4 PCB standoffs - the back row is pretty much cut off and (intentionally) mostly missing, ignore them. Of the front row, only the left and right most holes are used and line up with the PCB.

- Use M4 heat set inserts on the left and right most PCB stand offs at the front, leave the rest unused / empty

- Use M3 heatset inserts in the two screw holes used to hold the case together. Not sure how you're supposed to use them, but after installing them I ended up using (I think) a 2.5mm drill though the holes and down ~1cm, then running an M3 tap though the insert and into the hole so the length of screw used didn't matter as much.

- The back plate doesn't quite fit the way around I think it's supposed to fit due to the PCB being right up against the edge of the case - you need to have the side with cross pattern facing outwards

### Main board
Photo of board as it arrived from JLCPCB:

![main board]

* Solder four 20-pin pin sockets for the two Picos

* Configure jumpers. There is a serial interface on the "Serial" 3.5mm socket and on the 9pin accessory port - both can be independently changed between TTL & RS232 signalling. This is much easier to do before the front panel is attached and it's in a case!
My suggestion would be to have the 3.5mm socket as 3.3v TTL level as compatible serial cables are cheap on aliexpress. If you have one, a 2B cable should also work if set to TTL.

3.5mm set for TTL and accessory set for RS232:

![main board_jumpers]

* Fit the 4 transformers. These have the "P" facing towards the front of the case. Note that the transformers are used "backwards" - the side labelled primary goes to outputs.

* Thermistor: Attach two short wires to TH1 (under the left most Pico), and connect to a 10k thermistor. Later this will taped to the battery. For now, thread it through one of the slots in the PCB that lines up with a slot in the battery holder. I'd suggest using heat-shrink to insulate the legs of it.

![thermistor1]

Given that it's a large protected cell being charged with quite a low current, the thermistor probably isn't really essential. If you skip it, use a 10k resistor across TH1 instead (with nothing, the battery won't charge).


* Optional: If you want to use a 433 MHz transmitter with it, solder a 3-pin pin socket onto J9 (top, to the right of the battery holder)

Populated board, minus Picos:

![main board populated]


### Front panel
Follow appropriate notes depending on the version ordered:

* [Notes for v0.2 - round buttons](./Build_fp_0_2.md)
* [Notes for v1.1 - square buttons](./Build_fp_1_x.md)

### Miscellaneous
* Create a 10pin F-F cable for the display:

![10pin F-F cable]

(ok, so that looks pretty messy, but it works)

* Create a 2x4 way IDC cable for the front panel controls to main board:

![8w IDC]

* Push the front panel (with controls board already attached) onto the main board, then screw on the DB9 socket:

![front panel attached to main]


* Slot the boards into the case, and screw in place. There are two screw holes, one of which is hiding under the left most Pico
  - Hammond case: use screw Length (Excluding Head): 1/4" (6.5mm), Screw Size: No.6 (3.5mm)
  - 3D printed case: I used 6mm M4 screws

* Connect front panel to main board using IDC cable

* Connect LCD to main board using 10pin cable

* Copy main board firmware onto PicoW on the left (see section below), then plug into main board (USB socket pointing towards the right / towards case centre). This can be a bit fiddly

* Copy ZC624 firmware onto second Pico (see section below), then plug into the right most Pico socket, USB socket pointing towards the right / edge of the case

* Use kapton tape to secure the thermistor to the battery, then insert battery. This is a tight fit! I've found it helps to use pliers to flatten one of the contacts on the battery holder a bit first. The zc95 also might not power up on first switch on even if the battery has sufficient charge (due to battery protection). If this happens, briefly connect a charger then power it up. It should be fine after that until/unless the battery is removed/reinstalled.

![battery thermistor]


Assembled ZC95 should look something like this (3d printed & Hammond 1598DBK):

![zc95 assembled1] ![zc95 assembled2]

### The two Picos
I feel like some explanation here is required. The ZC95 Mk1 had the output stage on a separate daughter board ("ZC624") with its own Pico, in addition to a Pico on the main board. This Mk2 is very much an evolution of that design, so retains the two Picos, except everything has now been combined onto one PCB to make it smaller, as well as making assembly easier/cheaper.

Quick summary of what they do:
* Pico on the left, below the battery - UI, pattern generation / Lua interpreter, I/O (serial, accessory, triggers), WiFi, BLE, audio
* Pico in the top right - signal generation

Any references in the documentation/code/etc to either the "ZC624" or "Output module" is about the pico/circuitry now on the right hand side of the mainboard PCB.

### Loading firmware
Download firmware from [Releases](https://github.com/CrashOverride85/zc95/releases). Use the latest version, but certainly something >= v2.0-rc2

To load firmware onto a Pico:
* Hold down the BOOTSEL button
* Connect to PC via USB
* The Pico should appear as a USB mass storage device. Drag the appropriate uf2 firmware binary onto the drive:
  - zc95.uf2 - Pico on the left, below the battery (1)
  - OutputZc.uf2 - Pico at the top right (2)

{TODO}

### Power up
On power up, the LEDs should briefly turn purple (~2s), then turn green as the screen switches on and shows:

![zc95 powered up]

To vastly improve the accuracy of the battery gauge, I recommend leaving the box on until the battery is flat and it shuts off, then fully charging. 

# Troubleshooting
## Doesn't power on without charger attached
For the first power on after installing a battery, a USB-C charger needs to be connected - this is to be expected and not a fault.

Applies to PCB v2.3 (only): If the ZC95 doesn't power on after having being switched off for more than a few minutes, fit a 100uF capacitor (this is on the BoM) between the right most switch contact (as viewed from the top, with the switch towards you) and the top of the USB-C connector, like so:

![fix1 100uF]

If C64 & C65 are fitted, also remove them:

![fix1 C64 C65]

(these were removed from the BoM/CPL files 2025-08-18)

That should fix the issue. In the unlikely event it doesn't, try fitting something like a 22uF 1210 capacitor between the same switch contact as before and a ground on the bottom of the PCB, e.g.:

![fix1 22uF]


## Hardware check failed
If any of the I2C devices aren't detected, you should see an error similar to the below:

![hw check fail]

Here, it can't find the two ICs on the front panel (in this case, the IDC cable was unplugged).
There is serial debugging output on the "Accessory" DB9 connector (tx pin 3, ground pin 5) on the front panel. Either 3v3 TTL or RS232, depending on jumper settings.

## Battery gauge is wrong
If the battery gauge is showing filled green to indicate finished charging, but the percentage is some way off 100%, the best approach is to disconnect the charger, leave the box on until it shuts off due to low battery, then fully charge it (this should only need doing once). If you're impatient, removing the battery, leaving it a few minutes (maybe 15 to be sure), then reinstalling it and connecting the charger will probably improve it. But a full discharge/recharge cycle is best if you want it to be accurate. 

## Clearing saved settings / EEPROM
So far I've never found it necessary, but the EEPROM and user settings in flash can be reset to defaults by holding down the top right button and powering the box on. This will show a confirmation screen asking what should be reset:
* EEPROM - reset to defaults all settings that can be changed via the menus
* Flash  - clear any uploaded Lua scripts along with a section used by the btstack library for some bluetooth pairing data

Once reset, there should be a confirmation message, followed by flashing red lights. Power cycle the box, and the flash or EEPROM will have been reset to defaults.

Before showing the confirmation screen, the box will have confirmed the EEPROM IC can be detected, but no saved settings will have been used. 

## Flashing LEDs / no or corrupted display / output fault / general stability problems
For general erratic behaviour, the most likely cause is a problem with the battery - and is especially likely if the device sometimes powers up, but shows a red "?" in the battery indicator.

Even when powered via USB, a battery must still be present for reliable operation. If you're determined to run it without a battery, try putting a 470 uF (or higher) electrolytic capacitor in place of the battery.

## Output section
If the output section passes its self test, the OK LED, which is just above the left corner of the right most pico, should light (which should be ~1-2 seconds after power on). If the self test fails, this light will flash slowly (on for 0.5s, off for 0.5s etc).

There is also debugging output from the output pico on a 2 pin serial header next to it. Note that is at 3v3 level, and RS232 levels would damage it.

Connect a 3.3v TTL serial to USB adapter to the pins labelled Tx and GND on the header, connect at 115200 baud, and power it on. 
After the output of an I2C scan and a few other bits, it should output something similar to this on success:

```
calibrate for sm=0 OK: dac_val = 2860, voltage = 0.075732
calibrate for sm=1 OK: dac_val = 2870, voltage = 0.075732
calibrate for sm=2 OK: dac_val = 2870, voltage = 0.076538
calibrate for sm=3 OK: dac_val = 2840, voltage = 0.078149
Calibration success
```

On failure, the end of output may look something like this:

```
calibrate for sm=3 FAILED! final voltage = 0.011279, dac_value = 2600 (expecting 0.075v - 0.090v)
One or more chanel failed calibration, not enabling power.
CMessageProcess()
HALT.
```

### Self calibration notes
The purpose of the self calibration is to figure out exactly when the P-channel MOSFET for each channel starts to switch on, as this can vary slightly due to variances between parts, etc. It also serves as a basic self-test.

This probably needs a brief explanation about how the output stage is working. The two n-channel MOSFETs per chanel are used to generate +ve/-ve pulses (in the range of 10-255 microseconds). In normal operation, only one is switched on at once, but during calibration both are switched on so that there is little/no estim output.

The P-channel MOSFET is connected to the DAC via an op-amp, and is used to set the output power. With the maximum DAC value (4096) the MOSFET will (should) be fully OFF, and at 0 fully ON (i.e. maximum power).

In practice, if starting at a DAC value of 4000 and working down, the MOSFET should start to turn on at around 3000, and be fully on by ~1000.

During calibration, the voltage across the 0.5R sense resistor is measured, which corresponds to current flow though the P-channel MOSFET and transformer. This can be used to tell when the MOSFET starts to switch on.

The process for self power-on calibration (for each channel) is:
1. With the 3 MOSFETs switched off (dac value = 4000), the voltage across the sense resistor is measured - ideally it should be 0v as everything is off, but in reality noise means it won't quite be 0; however if it is > 0.03v something is clearly wrong so the calibration errors out.
2. The DAC is set to 3400
3. Both N-chanel MOSFETs are switched on
4. The voltage across the sense resistor is measured, then the N-channel fets switched off
    - If > 0.09v, something's gone wrong (unexpected jump in current), and calibration errors out
    - If > 0.075v, the DAC value is saved as the calibration value, and calibration for the channel completes successfully
    - Otherwise, the DAC value is reduced and the process repeats from step 3. If it's reached 2400 and the voltage still hasn't hit 0.075v, calibration errors out as the P-channel fet should be starting to switch on by this point.

All this means is that an error like this:
```
calibrate for sm=3 FAILED! final voltage = 0.011279, dac_value = 2400 (expecting 0.075v - 0.090v)
```
means that the PFET never switched on (enough) to complete calibration successfully.

An error like this:
```
calibrate for sm=3 FAILED! final voltage = 1.541235, dac_value = 3400 (expecting 0.075v - 0.090v)
```
means the opposite - at the starting value of 3400, the voltage across the sense resistor (and therefore current flow though the PFET & transformer) was already way above what it should be.

Possible causes (not exhaustive!) for calibration to fail:
* If all channels are showing a similar and very low voltage (~0.01v) at a DAC value of 2400, suspect the 9v supply
* Bad/incorrect PFET - e.g. not an IRF9Z24**NPBF**
* Too low value sense resistor (if DAC value is 2400), or too high (if DAC value is 3400)
* Incorrect resistor value in the opamp circuit - likely if the final voltage is wildly off. Also suspect a bad/cracked resistor or poor solder joint if the final voltage keeps changing between power cycles 
* If either chanel 1+2 or 3+4 fail with the same figures, take a close look at the corresponding ADC input pin on the Pico (pin 31/ADC0 for chan 1+2 or 32/ADC1 for chan 3+4) for bad solder joints. Also inspect the DAC (U7) and look for any bridged pins


**Note**: If calibration/self test fails, the 9v supply is switched off, so this not being present after a calibration failure is likely a symptom not a cause.

[main board]: images/main_board.jpg "Unpopulated main board"
[main board_jumpers]: images/main_board_jumpers.jpg "Main board with jumpers set"

[thermistor1]: images/thermistor1.jpg "Thermistor attached"


[main board populated]: images/main_board_populated.jpg "Populated main board"



[10pin F-F cable]: images/10pinFF.jpg "10 pin F-F cable"
[8w IDC]: images/8w_idc.jpg "2x4 IDC cable"
[display frame]: images/display_frame.jpg
[front panel attached to main]: images/fp_attached_to_main.jpg "Front panel attached to main board"
[battery thermistor]: images/battery_thermistor.jpg "Thermistor taped to battery"
[zc95 assembled1]: images/assembled_3dprinted.jpg "Fully assembled ZC95 in 3d printed case, minus cover"
[zc95 assembled2]: images/assembled_hammond.jpg "Fully assembled ZC95 in 1598DBK case, minus cover"
[zc95 powered up]: images/powered_up.jpg "Fully assembled ZC95 powered up"
[fp v1.1]: images/fp_v1.1.jpg "Front panel v1.1"
[hw check fail]: images/hw_check_fail.jpg "Power up error"
[fix1 C64 C65]: images/fix_c64_c65.jpg "C64 and C65 location on PCB"
[fix1 100uF]: images/fix_cap1.jpg "100uF capacitor between switch and USB-C"
[fix1 22uF]: images/fix_cap2.jpg "22uF capacitor on underside of PCB"
[q1]: images/build_Q1.jpg "Use diode for Q1"
[gh25]: https://github.com/CrashOverride85/zc95/discussions/25
[gh46]: https://github.com/CrashOverride85/zc95/issues/46

