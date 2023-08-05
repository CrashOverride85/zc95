

# Hardware build notes 

*Disclaimer*: These instructions are complete to the best of my knowledge, however I've only built the one so far, and that was over quite some time, so there may be omissions.

There is now an optional audio [input board](./AudioInput-Build.md) too.

## Prerequisites 
These notes assume a reasonable amount of experience assembling electronic kits. Other than through hole soldering, it also requires:
* Soldering 1 SMD part on the front panel control board. This is a SOIC-16 package, so fairly large / easy to solder
* Creating an IDC cable
* Crimping connectors for Molex KK style (well, clones in this case) connectors. Other 2.54mm connectors could be substituted.

## PCB + Parts ordering
### PCBs
For these 3 boards, order from JLCPCB using the linked gerbers and default settings (FR-4, 1.6mm thick, 1oz, etc.) except where noted:
* [Front panel](../pcb/FrontPanel.zip). Suggest ordering in black
* [Front panel controls](../pcb/FrontPanelControls.zip). Set "Remove Order Number" to "Specify a location" (location is already specified on the back of this board)
* [Main board](../pcb/MainBoard.zip)

The output board ("ZC624 Output module") has been designed with the JLCPCB SMT assembly service in mind, although the footprints are the hand-solder versions where applicable, and some (through hole) parts still need hand assembly.
Order the board using [these gerbers](../pcb/OutputModule/OutputBoard-gerbers.zip) and the default options as per the other boards.

This time, select the "SMT Assembly" option at the bottom - pick "Assemble top side" and Tooling holes "Added by JLCPCB". 
On the next page, add the BOM and CPL files:
* [output_bom_jlc.csv](../pcb/OutputModule/output_bom_jlc.csv)
* [output_cpl_jlc.csv](../pcb/OutputModule/output_cpl_jlc.csv)

The next page should show a list of all parts found / matched. Deselect the SS210 component (D8, D9, D10, D11, D12, D13, D14, D15), these aren't needed and reduce output power.

### PCB Parts
All parts required to populate the PCBs - with the exception of the transformers - can be purchased from LCSC, and this BOM spreadsheet lists required parts + quantity with the LCSC part number for each board on separate tabs:

[BoM (openoffice format)](BoM.fods)

As with the MK312-BT, the PFETs (IRF9Z24NPBF) in particular should not be substituted with anything similar, the exact part should be used.

### Misc parts
[The BoM spreadsheet](BoM.fods) includes a Misc tab with the rest of the parts required to complete the build (case, display, etc.).


## Assembly

### Main board
Photo of board as it arrived from JLCPCB:

![main board]

* Be sure not to mix up U3 (LM2941T, top left) and U6 (LM2576T-5, top middle), these have the same footprint and quite similar names
* Fit the IC sockets (if ordered) instead of directly soldering the ICs to the board
* J8 should have jumpers between pins 1+2, 3+4, 7+8, 9+10, like so:

  ![jumpers]

  Without these the serial port on the front won't work (but it won't affect anything else).
* Solder short (~12cm or so) wires on to J13, and fit spade connectors on the other end for the battery
* R14 & D4 (top middle) don't need to be populated. It's for a power LED that has no place on the front panel.
* J20 (I2C header, bottom left) is for future expansion and doesn't need to be populated
* J17 (just below battery, to the right) is for the optional audio input board; if building that, fit a 2.54mm pin header, otherwise leave unpopulated

Then (optionally) plug a 433MHz transmitter into J11

Populated board:

![main board populated]

### Output board ("ZC624 Output module")
Photo of board as it arrived from JLCPCB:

![zc624 smt only]

D8-D15 are populated in this photo, but these should be omitted when ordering the boards (or removed if ordered by mistake), as they reduce output power.

Notes
* The pin headers J1, J2 & J3 go on the bottom of the board, J4 ("Serial") on the top. I found it easier to plug the board into the main board, then solder J1, J2 & J3 to be sure they were straight / lined up
* The 4 transformers have the "P" facing inwards (towards each other). Note that the transformers are used "backwards" - the side labelled primary goes to outputs.
* PFETs Q2, Q5 & Q8 should have the metal back facing towards the bottom (transformer end) of the board. Q11 is the other way round (silkscreen is correct)
* Solder on both the 1x20 pin sockets for the Pico. The 3 pin header on the right should be unpopulated.

Fully assembled board:

![zc624 populated]


### Front panel controls 
Photo of board as it arrived from JLCPCB:

![front panel controls-bottom]
![front panel controls-top]

* All resistors are 10k, with the exception of R29 which is 100R (R29 is also the only resistor with its value labelled)
* Getting the LEDs at the correct height can be a little awkward. Suggest soldering the POTs + rotary encoder first, putting the LEDs in (no solder yet), then attaching the board to the front panel (using 20mm bolts). Make sure the LEDs are level-ish, then solder in place.
* Stating the obvious, but put the parts on the side indicated by the silkscreen (LEDs, POTs and rotary encoder one side, the rest the other)
* J1 should have the cut-out facing towards SW5 (rotary encoder)
* Pin 1 of the ADC (U1, PCF8591) is the top left (with board orientated with the `ZC95-FrontPanel-Analog(back)` text at the top)


Fully assembled board:

![front panel controls-bottom-populated]
![front panel controls-top-populated]

### Front panel
Photo of board as it arrived from JLCPCB:

![front panel]

* Perhaps the most annoying part of the whole build is creating a cable for the 4 buttons. The buttons are named like so:

![front panel buttons]

Wire each button back to a connector in the position that matches the silkscreen (first two positions are button A, next two button B, etc.)
Then attach the buttons to the front panel.

* Attach connector to back of LCD, then LCD to the front panel with M2, 12mm bolts

The board should look something like this:

![front panel back]

Finally, attach the front panel controls board using 20mm bolts. The completed assembly should look something like:

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

## ZC624 output module
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
calibrate for sm=3 FAILED! final voltage = 0.011279, dac_value = 2600 (expecting 0.075v - 0.090v)
```
means that the PFET never switched on (enough) to complete calibration successfully.

An error like this:
```
calibrate for sm=3 FAILED! final voltage = 1.541235, dac_value = 3400 (expecting 0.075v - 0.090v)
```
means the opposite - at the starting value of 3400, the voltage across the sense resistor (and therefore current flow though the PFET & transformer) was already way above what it should be.

Possible causes (not exhaustive!) for calibration to fail:
* If all channels are showing a similar and very low voltage (~0.01v) at a DAC value of 2600, suspect the 9v supply (and in turn, the 12v supply it's derived from)
* Bad/incorrect PFET - e.g. not an IRF9Z24**NPBF**
* Too low value sense resistor (if DAC value is 2600), or too high (if DAC value is 3400)
* Incorrect resistor value in the opamp circuit - likely if the final voltage is wildly off. Also suspect a bad/cracked resistor or poor solder joint if the final voltage keeps changing between power cycles 


**Note**: If calibration fails, the 9v supply is switched off, so this not being present after a calibration failure is not a fault.

[jumpers]: images/jumpers.jpg "J8 with jumpers fitted"
[main board]: images/main_board.jpg "Unpopulated main board"
[main board populated]: images/main_board_populated.jpg "Populated main board"
[zc624 smt only]: images/zc624.jpg "ZC624 as received from JLCPCB"
[zc624 populated]: images/zc624_populated.jpg "Fully populated ZC624"
[front panel controls-bottom]: images/fpc_bottom.jpg "Unpopulated front panel controls board - bottom"
[front panel controls-top]: images/fpc_top.jpg "Unpopulated front panel controls board - top"
[front panel controls-bottom-populated]: images/fpc_bottom_populated.jpg "Populated front panel controls board - bottom"
[front panel controls-top-populated]: images/fpc_top_populated.jpg "Populated front panel controls board - top"
[front panel]: images/fp.jpg "Front panel"
[front panel buttons]: images/fp-abcd.jpg "Front panel with buttons labelled"
[front panel back]: images/fp_back.jpg "Back of front panel with LCD and buttons attached"
[front panel assembled]: images/fp_assembled.jpg "Front panel with LCD, buttons and controls attached"
[10pin F-F cable]: images/10pinFF.jpg "10 pin F-F cable"
[8w IDC]: images/8w_idc.jpg "2x4 IDC cable"
[front panel attached to main]: images/fp_attached_to_main.jpg "Front panel attached to main board"
[zc95 assembled]: images/assembled.jpg "Fully assembled ZC95, minus cover"
[zc95 powered up]: images/powered_up.jpg "Fully assembled ZC95 powered up"
[hw check fail]: images/hw_check_fail.jpg "Power up error"



