# Output board

The original ZC95 mk1 had the estim output stage on a separate PCB; that's been incorporated into the main board with the mkII. Whilst it was only ever intended to be used as a part of the ZC95, the output board could also be used from other projects, and there is an included [Arduino library](../Arduino/libraries/Zc624Output/README.md) for it.
So these are the (slightly tweaked) original build notes from that board, in case it's still of interest to anyone.

---

## PCBs
The output board ("ZC624 Output module") has been designed with the JLCPCB SMT assembly service in mind, although the footprints are the hand-solder versions where applicable, and some (through hole) parts still need hand assembly.
Order the board using [these gerbers](./pcb/OutputBoard-gerbers.zip) and the default options as per the other boards.

This time, select the "SMT Assembly" option at the bottom - pick "Assemble top side" and Tooling holes "Added by JLCPCB".
On the next page, add the BOM and CPL files:
* [output_bom_jlc.csv](./pcb/output_bom_jlc.csv)
* [output_cpl_jlc.csv](./pcb/output_cpl_jlc.csv)

The next page should show a list of all parts found / matched. The BoM/CPL file also includes some through hole parts - I'd suggest un-ticking these and soldering them yourself as it's cheaper.

## Other parts
Other parts required to assemble the board are listed in the [BoM](OutputModuleBom.fods).

## Transformers
The ZC624 output board was originally designed and tested with 42TL004 transformers in mind. However, v0.2 is designed so that the larger/more powerful 42TU200 transformers _should_ also fit. This isn't something I've tested. I recommend sticking with the 42TL004's listed in the BoM.

## Assembly
Photo of board as it arrived from JLCPCB:

![zc624 smt only]

Notes
* The pin headers J1, J2 & J3 go on the bottom of the board, J4 ("Serial") on the top. I found it easier to plug the board into the main board, then solder J1, J2 & J3 to be sure they were straight / lined up
* The 4 transformers have the "P" facing towards the bottom. Note that the transformers are used "backwards" - the side labelled primary goes to outputs.
* PFETs Q2, Q5, Q8 & Q11 should have the metal back facing towards the left of the board (silkscreen is correct)
* Solder on both the 1x20 pin sockets for the Pico. The 3 pin header on the right should be unpopulated.

Fully assembled board:

![zc624 populated]

## Loading firmware
Download the "OutputZc.uf2" firmware from [Releases](https://github.com/CrashOverride85/zc95/releases).

To load firmware onto a Pico:
* Hold down the BOOTSEL button
* Connect to PC via USB
* The Pico should appear as a USB mass storage device. Drag the uf2 firmware binary onto the drive

## Troubleshooting
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


[zc624 smt only]: zc624.jpg "ZC624 as received from JLCPCB"
[zc624 populated]: zc624_populated.jpg "Fully populated ZC624"
