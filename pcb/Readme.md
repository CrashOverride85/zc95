# Changes

## Front panel

### v0.5 (Mk2 only)
- Updated for Mk2: USB-C, separate audio & serial sockets, output sockets changed from 2.5mm to 3.5mm

### v0.4
- *Not publicly released*

### v0.3 (Mk1 only)
- Button holes enlarged to 8.1mm for LP1OA1Ax illuminated buttons

### v0.2 (Mk1 only)
- Left most 3.5mm socket renamed from "Serial" to "Aux"
- 4x holes for potentiometers enlarged so RK09L1140A2U fits without needing to manually enlarge the hole
- Project updated to KiCad 6

### v0.1 (Mk1 only)
First public version

## FrontPanelControls
### v0.2 (Mk1 & Mk2)
- Requires firmware version >=1.8
- Now mostly SMD aimed at JLCPCB's SMT assembly service
- 4x front panel buttons are now connected to this board, not the main board
- ADC changed from PCF8591 (EoL) to ADS1115
- I/O expander changed from PCF8574 to TCA9534
- Allows use of illuminated buttons (LP1OA1Ax) controlled by TLC59108
- Project updated to KiCad 6

### v0.1 (Mk1 only)
First public version

## MainBoard

### v2.5
Fix:
 - Update power switch so that it toggles the power enable pin of the voltage regulators, instead of directly switching the power as before. To avoid over current fault / shutdown of BQ25601.

 Tweaks:
 - Changed 500mΩ sense resistors from 1218 to 2512 footprint for availability reasons
 - Sleeve and Ring of output TRS jacks are now connected together. Previously Ring was unconnected.

### v2.4
- *Not publicly released*

### v2.3
Tweaks:
 - No more vias in pads
 - Changed charge controller from TP4056 to BQ25601
 - Added 3.3v LDO powered from 5v rail to supply some of the audio circuitry (reduces noise when using a mic)
 - Changed footprint of LCD and Debug connectors to make intended orientation clear
 - Added ability to sense voltage on USB CC lines to determine type of charger connected
 - Display reset line now connected to port expander instead of pull up to 5v. Fixes intermittent issue where display would show pure white if the box was powered off, then powered up a few seconds later.

### v2.2
- *Not publicly released*

### v2.1
Fixed solder mask minimum width setting - restore to KiCad default of 0

### v2.0
Mk2. Many changes:
 - Now includes functionality that was previously on separate output & audio boards
 - USB-C charger input instead of 15v barrel jack
 - 26650 cell instead of 12v SLA battery (similar capacity)
 - "Aux" socket swapped for separate "Serial" and "Audio" sockets
 - Output sockets changed from 2.5mm to 3.5mm
 - Serial selectable (via jumpers) between 3.3v TTL and RS232
 - Display no longer briefly flashes white on power on
 - Removed: 12v output on accessory port
 - Will not work with FrontPanelControls PCB v0.1

### v0.2
Mk1. First public version.
