# Hardware build notes for audio input board

## Changes to ZC95 main board
Two changes are required:
  1. Remove the jumpers on J8
  2. Fit a 10 way 2.54mm pin header on J17

## Ordering
Parts required for this board are on the "Audio input" tab of the [BoM (openoffice format)](BoM.fods) spreadsheet. 
When building the board, there are two choices for gain control:

1. Internal adjustment only - fit RV1 and RV2
2. (Recommended) Variable from menu - Fit U1. Also allows the presence of the board to be auto-detected

The downside of fitting U1 is cost, plus it's an SMD part (and whilst not hard, it's also not the easiest part to solder if you've never done anything SMD before). Also it's the sort of part that might go out of stock at any moment...

Order the blank PCB from JLC using the [AudioInput](../pcb/AudioInput.zip) gerbers and the default settings (FR-4, 1.6mm thick, 1oz, etc.)

## Assembly notes

Unfortunately I didn't manage to fit the component values on the silkscreen, so the best way to assemble the board is probably just going through the BoM which lists component references & values; a printout of the [board outline and silkscreen](./AudioOutput.pdf) may help.

Mistakes on silkscreen of v0.3 PCB: 
- The footprints for Q1 & Q3 (P Channel MOSFETs) are backwards
- Didode D3 (under the relay) is also backwards - the cathode should go in the hole labelled A

Assembled board:

![AI board]

**TODO**: Fix photo, D3, Q1 & Q3 are wrong

Once assembled, plug the audio board into the main board, with the text ("Audio input for ZC95") towards the front / near the front panel. Be careful to line it up correctly with the headers, it is possible to insert it misaligned. 

If the digipot was fitted (U1) it should "just work", and the audio pattern(s) and "Audio input" menu should appear. If it wasn't fitted use the hardware config menu (see [audio operation notes](./AudioInput-Operation.md)) to set the Audio to "On (no gain)".

## Brief description
Hopefully not needed for the assembly & use of the board, but maybe a few notes about what this board is doing is good idea.

Without the board fitted, the jumpers fitted to J8 on the main board send the RS232 output from the UT232A IC to the 3.5mm "Aux" socket on the front panel. With the board fitted, the relay can be used to switch the Aux socket between the UT232A, and (ultimately) the ADC inputs present on header J1.

This somewhat shoddy diagram shows a high level overview of what's going on:

![AI diagram]

The firmware uses the presence of the digipot (U1, used for gain control) to detect the board and show the audio options. But this can be overridden from the Hardware Config menu, see [audio operation notes](./AudioInput-Operation.md).

One more thing maybe worthy of note is microphones... as this is an area where I didn't know what I didn't know!

There seem to be two common types of microphones (ignoring expensive stuff):

1. Electret microphones - typical PC style mics, including lavalier mics. These either require a small amount of power supplied to operate (in the region of 2v - 5v), or are self powered and need batteries / charging. Google suggests that electret mics generally have this bias power power on the ring, with signal on the tip. But of the two I tested, they both needed power on the tip. So like the mk312bt, this board also supplies power on tip+ring when enabled. There seem to be a few pinouts to consider when you get into stereo mic inputs, combined headset + mic inputs (4 conductor jacks), etc.

2. Dynamic microphones - think a hand held karaoke mic, that sort of thing. Produce a weaker signal than electret mics, but require no power (**and power could damage them**). Typically have a 6.35mm jack, but not always.

This audio input board works best with electret mics - there's not quite enough gain for dynamic mics - although R9 & R10 in the mic preamp could almost certainly be adjusted to make them work well enough.


[AI board]: images/ai_populated.jpg "Assembled audio input board"
[AI diagram]: images/AudioInput.png "Audio input diagram"
