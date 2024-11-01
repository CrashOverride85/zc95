# Hardware build notes 

There is now an optional audio [input board](./AudioInput-Build.md) too.

## Prerequisites 
These notes assume a reasonable amount of experience assembling electronic kits. I suggest having a read though these notes before ordering anything.

## PCB + Parts ordering
### PCBs
For these 2 boards, order from JLCPCB using the linked gerbers and default settings (FR-4, 1.6mm thick, 1oz, etc.) except where noted:
* [Front panel](../pcb/FrontPanel.zip). Suggest ordering in black
* [Main board](../pcb/MainBoard.zip)

#### Output board
The output board ("ZC624 Output module") has been designed with the JLCPCB SMT assembly service in mind, although the footprints are the hand-solder versions where applicable, and some (through hole) parts still need hand assembly.
Order the board using [these gerbers](../pcb/OutputModule/OutputBoard-gerbers.zip) and the default options as per the other boards.

This time, select the "SMT Assembly" option at the bottom - pick "Assemble top side" and Tooling holes "Added by JLCPCB". 
On the next page, add the BOM and CPL files:
* [output_bom_jlc.csv](../pcb/OutputModule/output_bom_jlc.csv)
* [output_cpl_jlc.csv](../pcb/OutputModule/output_cpl_jlc.csv)

The next page should show a list of all parts found / matched. The BoM/CPL file also includes some through hole parts - I'd suggest un-ticking these and soldering them yourself as it's cheaper.

#### Front panel controls
v0.2 of this board is also designed with the JLCPCB assembly service in mind, with a few though hole parts also requiring hand assembly

Order the board using [these gerbers](../pcb/FrontPanelControls-v0.2/GERBER-PanelControls.zip) and the default options as per the other boards.

Select the "SMT Assembly" option again at the bottom - pick "Assemble **BOTTOM** side" (this is _not_ the default) and Tooling holes "Added by JLCPCB". 
On the next page, add the BOM and CPL files:
* [BOM-PanelControls.csv](../pcb/FrontPanelControls-v0.2/BOM-PanelControls.csv)
* [CPL-PanelControls.csv](../pcb/FrontPanelControls-v0.2/CPL-PanelControls.csv)

The next page should show a list of all parts found / matched. The BoM/CPL file also includes the through hole part J5 - I'd suggest un-ticking it and soldering it yourself as it's cheaper.

### Transformers
The ZC624 output board was originally designed and tested with 42TL004 transformers in mind. However, v0.2 is designed so that the larger/more powerful 42TU200 transformers _should_ also fit. Unfortunately, at the time of writing, these are out of stock, so is completely untested. I recommend sticking with the 42TL004's listed in the BoM. 

### PCB Parts
All parts required to populate the PCBs - with the exception of the transformers - can be purchased from LCSC, and this BOM spreadsheet lists required parts + quantity with the LCSC part number for each board on separate tabs:

[BoM (openoffice format)](BoM.fods)

As with the MK312-BT, the PFETs (IRF9Z24NPBF) in particular should not be substituted with anything similar, the exact part should be used.

### Misc parts
[The BoM spreadsheet](BoM.fods) includes a Misc tab with the rest of the parts required to complete the build (case, display, etc.).

### LED riser
_Optional_: Order or 3d print LED riser: [STL file](../misc/led-riser/led-riser.stl).

## Assembly

### Main board
Photo of board as it arrived from JLCPCB:

![main board]

* For Q1 (BS250), fit a 1N5819 diode instead, as shown:

  ![q1]

  (See [#25][gh25] & [#46][gh46] if interested in why)

* Be sure not to mix up U3 (LM2941T, top left) and U6 (LM2576T-5, top middle), these have the same footprint and quite similar names
* Fit the IC sockets (if ordered) instead of directly soldering the ICs to the board
* J8 should have jumpers between pins 1+2, 3+4, 7+8, 9+10, like so:

  ![jumpers]

  Without these the serial port on the front won't work (but it won't affect anything else).
* Solder short (~12cm or so) wires on to J13, and fit spade connectors on the other end for the battery
* R14 & D4 (top middle) don't need to be populated. It's for a power LED that has no place on the front panel.
* J20 (I2C header, bottom left) is for future expansion and doesn't need to be populated
* J17 (just below battery, to the right) is for the optional audio input board; if building that, fit a 2.54mm pin header, otherwise leave unpopulated
* J16 "FP-BUTTONS" (below the battery, towards the centre) is no longer used (was used for v0.1 of the front panel), and can be left unpopulated
* J19 IDC connector should be orientated with the cut out towards the back of the board, like so:

  ![main board idc]


Then (optionally) plug a 433MHz transmitter into J11

Populated board:

![main board populated]

### Output board ("ZC624 Output module")
Photo of board as it arrived from JLCPCB:

![zc624 smt only]

Notes
* The pin headers J1, J2 & J3 go on the bottom of the board, J4 ("Serial") on the top. I found it easier to plug the board into the main board, then solder J1, J2 & J3 to be sure they were straight / lined up
* The 4 transformers have the "P" facing towards the bottom. Note that the transformers are used "backwards" - the side labelled primary goes to outputs.
* PFETs Q2, Q5, Q8 & Q11 should have the metal back facing towards the left of the board (silkscreen is correct)
* Solder on both the 1x20 pin sockets for the Pico. The 3 pin header on the right should be unpopulated.

Fully assembled board:

![zc624 populated]


### Front panel controls 
Photo of board as it arrived from JLCPCB:

![front panel controls-bottom]
![front panel controls-top]


* If printed/ordered, use the LED risers to get the LEDs at the correct height, e.g.:

  ![front panel LED riser]:

  (Thanks to @electro991 for this, see [#102][gh102])

* Otherwise, getting the LEDs at the correct height can be a little awkward. Suggest soldering the POTs + rotary encoder first, putting the LEDs in (no solder yet), then attaching the board to the front panel (using 20mm bolts). Make sure the LEDs are level-ish, then solder in place.
* Stating the obvious, but put all the hand-solder parts (LEDs, POTs and rotary encoder) on the side indicated by the silkscreen

Fully assembled board:

![front panel controls-top-populated]

### Front panel
Photo of board as it arrived from JLCPCB:

![front panel]

Perhaps the most annoying part of the whole build is wiring the 4 buttons to the front panel. The buttons should be attached to the corresponding position on the front panel controls PCB:

![front panel buttons]

If using the illuminated LP1OA1Ax buttons, there are 4 wires per button - 2x for switch contacts and 2x for the LED. The LED part of the buttons should be connected like this:

![button connections]

The pin marked with the white dot is the LED cathode, the opposite pin is the anode, the other two pins are the switch contacts.

Once connected, it should look something like:

![front panel buttons connected]

Use the M2 nuts & 20mm bolts to attach the board to the front panel, then screw on the washers & nuts for the potentiometers. 

Attach connector to back of LCD, then LCD to the front panel with M2 nuts, 12mm bolts.

The assembled front panel should look something like this:

![front panel back]

![front panel assembled]

### Miscellaneous
* Create a 10pin F-F cable for the display:

![10pin F-F cable]

(ok, so that looks pretty messy, but it works)

* Create a 2x4 way IDC cable for the front panel controls to main board:

![8w IDC]

* Push the front panel (with controls board already attached) onto the main board, then screw on the DB9 socket:

![front panel attached to main]

* Slot the boards into the case, and screw in place: Screw Length (Excluding Head): 1/4" (6.5mm), Screw Size: No.6 (3.5mm)

* Stick the battery down. Suggest using 3M double sided tape (MNT-FT24MM-16FT)

* Connect front panel to main board using IDC cable

* Connect LCD to main board using 10pin cable

* Copy main board firmware onto first Pico, or a Pico W if wanting remote access (see section below), then plug into main board (USB socket pointing towards the left / nearest case edge)

* Copy ZC624 firmware onto second Pico (see section below), then plug into ZC624 board (USB socket pointing towards the left)

* Plug the ZC624 into the main board. Be careful to line it up correctly as it is possible to plug it in misaligned which would be bad

* Connect battery

Assembled ZC95 should look like:

![zc95 assembled]

### Loading firmware
Download firmware from [Releases](https://github.com/CrashOverride85/zc95/releases).

To load firmware onto a Pico:
* Hold down the BOOTSEL button
* Connect to PC via USB
* The Pico should appear as a USB mass storage device. Drag the appropriate uf2 firmware binary onto the drive:
  - zc95.uf2 - Main board firmware
  - OutputZc.uf2 - ZC624 firmware

And don't mix the two up!


### Power up
On power up, the LEDs should briefly turn purple (~2s), then turn green as the screen switches on and shows:

![zc95 powered up]

# Troubleshooting
## ZC95 main board
If any of the I2C devices aren't detected, you should see an error similar to the below:

![hw check fail]

Here, it can't find the two ICs on the front panel (in this case, the IDC cable was unplugged).
There is serial debugging output on the "Accessory" DB9 connector (tx pin 3, ground pin 5) on the front panel at RS232 levels.

### Clearing saved settings / EEPROM
So far I've never found it necessary, but the EEPROM and user settings in flash can be reset to defaults by holding down the top right button and powering the box on. This will show a confirmation screen asking what should be reset:
* EEPROM - reset to defaults all settings that can be changed via the menus
* Flash  - clear any uploaded Lua scripts along with a section used by the btstack library for some bluetooth pairing data

Once reset, there should be a confirmation message, followed by flashing red lights. Power cycle the box, and the flash or EEPROM will have been reset to defaults.

Before showing the confirmation screen, the box will have confirmed the EEPROM IC can be detected, but no saved settings will have been used. 

## ZC624 output module
If the ZC624 passes its self test, the OK LED should light (which should be ~1-2 seconds after power on). If it fails, this light should flash.

There is also debugging output from the ZC624 board on the serial header. Note that is at 3v3 level, and RS232 levels would damage it.

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

This probably needs a brief explanation about how the output board is working. The two n-channel MOSFETs per chanel are used to generate +ve/-ve pulses (in the range of 10-255 microseconds). In normal operation, only one is switched on at once, but during calibration both are switched on so that there is little/no estim output.

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
* If all channels are showing a similar and very low voltage (~0.01v) at a DAC value of 2400, suspect the 9v supply (and in turn, the 12v supply it's derived from)
* Bad/incorrect PFET - e.g. not an IRF9Z24**NPBF**
* Too low value sense resistor (if DAC value is 2400), or too high (if DAC value is 3400)
* Incorrect resistor value in the opamp circuit - likely if the final voltage is wildly off. Also suspect a bad/cracked resistor or poor solder joint if the final voltage keeps changing between power cycles 


**Note**: If calibration fails, the 9v supply is switched off, so this not being present after a calibration failure is not a fault.

[jumpers]: images/jumpers.jpg "J8 with jumpers fitted"
[main board]: images/main_board.jpg "Unpopulated main board"
[main board populated]: images/main_board_populated.jpg "Populated main board"
[main board idc]: images/main_board_idc.jpg "IDC socket on main board"
[zc624 smt only]: images/zc624.jpg "ZC624 as received from JLCPCB"
[zc624 populated]: images/zc624_populated.jpg "Fully populated ZC624"
[front panel controls-bottom]: images/fpc_bottom.jpg "Unpopulated front panel controls board - bottom"
[front panel controls-top]: images/fpc_top.jpg "Unpopulated front panel controls board - top"
[front panel controls-top-populated]: images/fpc_top_populated.jpg "Populated front panel controls board - top"
[front panel LED riser]: images/fpc_led_riser.jpg "Front panel with LED riser"
[front panel]: images/fp.jpg "Front panel"
[front panel buttons connected]: images/fpc_buttons.jpg "Front panel with buttons attached"
[button connections]: images/button_connection.png "Front panel buttons to fpc board connection"
[front panel buttons]: images/fp-abcd.jpg "Front panel with buttons labelled"
[front panel back]: images/fp_back.jpg "Back of front panel with LCD and buttons attached"
[front panel assembled]: images/fp_assembled.jpg "Front panel with LCD, buttons and controls attached"
[10pin F-F cable]: images/10pinFF.jpg "10 pin F-F cable"
[8w IDC]: images/8w_idc.jpg "2x4 IDC cable"
[front panel attached to main]: images/fp_attached_to_main.jpg "Front panel attached to main board"
[zc95 assembled]: images/assembled.jpg "Fully assembled ZC95, minus cover"
[zc95 powered up]: images/powered_up.jpg "Fully assembled ZC95 powered up"
[hw check fail]: images/hw_check_fail.jpg "Power up error"
[q1]: images/build_Q1.jpg "Use diode for Q1"
[gh25]: https://github.com/CrashOverride85/zc95/discussions/25
[gh46]: https://github.com/CrashOverride85/zc95/issues/46
[gh102]: https://github.com/CrashOverride85/zc95/discussions/102
