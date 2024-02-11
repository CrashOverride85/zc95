# Changes

## Front panel
### v0.3
- Button holes enlarged to 8.1mm for LP1OA1Ax illuminated buttons

### v0.2
- Left most 3.5mm socket renamed from "Serial" to "Aux"
- 4x holes for potentiometers enlarged so RK09L1140A2U fits without needing to manually enlarge the hole
- Project updated to KiCad 6

### v0.1
First public version

## FrontPanelControls
### v0.2
- Requires firmware version >=1.8
- Now mostly SMD aimed at JLCPCB's SMT assembly service
- 4x front panel buttons are now connected to this board, not the main board
- ADC changed from PCF8591 (EoL) to ADS1115
- I/O expander changed from PCF8574 to TCA9534
- Allows use of illuminated buttons (LP1OA1Ax) controlled by TLC59108
- Project updated to KiCad 6

### v0.1
First public version

## MainBoard
### v0.2
First public version

## OutputModule
### v0.2
- Fixed snubber. Swapped diode for diode + TVS diode (thanks to Onwrikbaar for suggestion)
- _Might_ now also work with larger 42TU200 transformers (currently out of stock, so unable to test)
- Increased resistor values for status LEDs to reduce brightness
- Swapped 4x 100uF through hole electrolytic capacitors for 4x 220uF tantalum capacitors
- Project updated to KiCad 6

### v0.1
First public version

## AudioInput
### v0.3
First public version
