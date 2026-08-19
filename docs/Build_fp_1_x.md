 # Front panel v1.x
 
Ordering & build instructions for new front panel kindly designed and contributed by rootuz. This should result in a zc95 that looks like this:

![fp v1.1]

Please note that whilst the project files & gerbers for these PCBs are available, they are provided under the [CERN-OHL-S](../pcb-project/FrontPanelControls/v1_x/LICENSE.txt) licence.

## Order PCBs

### Front panel controls
Order the board using [these gerbers](../pcb/FrontPanel_v1_x/FrontPanelControls/ZC95_FrontPanelControls_v1_1_Gerber.zip) and the default options as per the main board.

On the next page, add the BOM and CPL files:
* [BOM-PanelControls.csv](../pcb/FrontPanel_v1_x/FrontPanelControls/ZC95_FrontPanelControls_v1_1_BOM.csv)
* [CPL-PanelControls.csv](../pcb/FrontPanel_v1_x/FrontPanelControls/ZC95_FrontPanelControls_v1_1_CPL.csv)

### Front panel
Order the board using [these gerbers](../pcb/FrontPanel_v1_x/FrontPanel.zip). Not essential, but as the board forms the front of the case, I would suggest
 **Order in black**; or at least, think about what colour you want the front panel to be and don't just go with the default of green unless that's what you really want

Keep the other options as default.

## 3D printed parts

### Display frame
Optional: Allows the screws holding the Adafruit display to the front panel to be tightened without crushing the display. [STL file](../misc/display-frame/lcd_frame_ada.stl).

## Boards

Photo of front panel as it arrived from JLCPCB:

![fp]

Photo of front panel controls board as it arrived from JLCPCB:

![fp front]
![fp back]


## Assembly

* Solder on a 2x4 IDC socket on the rear of the board for "P1" - make sure orientation matches the silkscreen.

* Solder a 10 pin molex KK style connector for "H1" ("Mainboard LCD").

* Solder 10 way pin header onto back on LCD

* If ordered/printed, fit the display frame to the display:
  
  ![display frame]

* Use 4x bolts to attach the display (& optional frame) to the front panel, and secure with nuts

* Remove all nuts & washers from the potentiometers & rotary encoder, and push the front panel onto the controls board, lining up display header with PCB

* Ensure the rear/controls PCB is level (all buttons are level), and solder display in place

* Add remaining nuts for display/pcb, and add the 4x nuts & bolts around the potentiometers & rotary encoder

* Push the 6x light pipes into front panel. This should require some force.


The assembled front panel should look something like this:

![fp assembled1]

![fp assembled2]



[fp v1.1]: images/fp_v1.1.jpg "Front panel v1.1"
[fp front]: images/fp_v1_x_front.jpg "Front panel controls PCB, top side as it arrived from JLCPCB" 
[fp back]: images/fp_v1_x_back.jpg "Front panel controls PCB, bottom side as it arrived from JLCPCB" 
[fp]: images/fp_v1_x.jpg "Front panel as it arrived from JLCPCB"
[display frame]: images/display_frame.jpg


[fp assembled1]: images/fp_v1_x-assembled1.jpg "Front panel, assembled, top down view " 
[fp assembled2]: images/fp_v1_x-assembled2.jpg "Front panel, assembled, rear view" 
