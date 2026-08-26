 # Front panel v0.2
 
Ordering & build instructions for front panel v0.2, which should result in a zc95 that looks like this:

![zc95 powered up]

## Order PCBs

### Front panel controls
Order the board using [these gerbers](../pcb/FrontPanel_v0_2/FrontPanelControls/GERBER-PanelControls.zip) and the default options as per the main board.

Select the "SMT Assembly" option again at the bottom - pick "Assemble **BOTTOM** side" (this is _not_ the default) and Tooling holes "Added by JLCPCB". 
On the next page, add the BOM and CPL files:
* [BOM-PanelControls.csv](../pcb/FrontPanel_v0_2/FrontPanelControls/BOM-PanelControls.csv)
* [CPL-PanelControls.csv](../pcb/FrontPanel_v0_2/FrontPanelControls/CPL-PanelControls.csv)

### Front panel
Order the board using [these gerbers](../pcb/FrontPanel_v0_2/FrontPanel.zip). Not essential, but as the board forms the front of the case, I would suggest:
* **Order in black**; or at least, think about what colour you want the front panel to be and don't just go with the default of green unless that's what you really want
* For "Mark on PCB" in the "High-spec Options" section, pick "Order Number (Specify Position)". This is makes sure the order number is put on the back of the PCB (the placeholder is already on the back of the board for this)

Keep the other options as default.

### 3D printed parts

Both of these are optional.

#### LED riser
Order or 3d print LED riser: [STL file](../misc/led-riser/led-riser.stl). These make it easier to install the LEDs at the correct/consistent height.

#### Display frame
Allows the screws holding the Adafruit display to the front panel to be tightened without crushing the display. [STL file](../misc/display-frame/lcd_frame_ada.stl).


### Misc ordering notes

* If substituting parts, be aware that not all WS2812D LEDs have the pinout

* Buttons: Consider what colour you want the buttons. The BoM lists LP1OA1A**B** for blue (as I used for the Mk1), but for this Mk2 I've gone with LP1OA1A**R** for red. Still not sure which I prefer ¯\_(ツ)_/¯

## Assembly

### Front panel controls 
* Photo of board as it arrived from JLCPCB:

![front panel controls-bottom]
![front panel controls-top]


* If printed/ordered, use the LED risers to get the LEDs at the correct height, e.g.:

  ![front panel LED riser]:

  (Photo shows front panel attached to a mk1)

  Thanks to @electro991 for this, see [#102][gh102].

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

_Optional_: If you ordered/printed the display frame, place it around the display before mounting to the front panel:

![display frame]

(Thanks to rootuz for this frame)

Use the M2 nuts & 20mm bolts to attach the board to the front panel, then screw on the washers & nuts for the potentiometers. 

Attach connector to back of LCD, then LCD to the front panel with M2 nuts, 12mm bolts.

The assembled front panel should look something like this:

![front panel back]



[button connections]: images/button_connection.png "Front panel buttons to fpc board connection"
[display frame]: images/display_frame.jpg
[front panel]: images/fp.jpg "Front panel"
[front panel buttons]: images/fp-abcd.jpg "Front panel with buttons labelled"
[front panel back]: images/fp_back.jpg "Back of front panel with LCD and buttons attached"
[front panel buttons connected]: images/fpc_buttons.jpg "Front panel with buttons attached"
[front panel controls-bottom]: images/fpc_bottom.jpg "Unpopulated front panel controls board - bottom"
[front panel controls-top]: images/fpc_top.jpg "Unpopulated front panel controls board - top"
[front panel controls-top-populated]: images/fpc_top_populated.jpg "Populated front panel controls board - top"
[front panel LED riser]: images/fpc_led_riser.jpg "Front panel with LED riser"
[zc95 powered up]: images/powered_up.jpg "ZC95 with v0.2 front panel"
[gh102]: https://github.com/CrashOverride85/zc95/discussions/102
