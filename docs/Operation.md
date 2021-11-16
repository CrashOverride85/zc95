# Operation


## Device layout
![device layout]

Buttons 1 - 4 are soft buttons and the purpose changes dependant on the screen. 

| Number | Description
| ------ | -----------
| 1      | Softbutton - Used to select pattern from the main menu, and changes dependant on the selected pattern.
| 2      | Softbutton - Usually back / return to previous screen
| 3      | Softbutton - Usually scroll up
| 4      | Softbutton - Usually scroll down
| 5      | Power. Left is off, right is on. Probably should have labelled that.
| 6      | Power in. Should be centre positive 15v DC
| 7      | Serial port. Currently only debugging output (RS232 level). Tip is TX / output from box, Sleeve is ground.
| 8      | Accessory port. Include power, serial and 3x GPIO pins. See "Accessory port" in misc section. Currently not really used by much.
| 9      | Trigger input 1. Shorting either Sleeve or Ring to Tip registers trigger. The box can differentiate between S-T and R-T triggers (i.e. each trigger socket can react to two different inputs), however this capability is unused.
| 10     | Trigger input 2. As above, except currently unused.
| 11     | 2.5mm socket for channel 1 output
| 12     | 2.5mm socket for channel 2 output
| 13     | 2.5mm socket for channel 3 output
| 14     | 2.5mm socket for channel 4 output
| 15     | Adjust - Used to change the setting of on-screen sliders / multi choice options. A blue bar/background in the middle/left section of the screen indicates when the Adjust control can be used
| 16     | Power control for channel 1
| 17     | Power control for channel 2
| 18     | Power control for channel 3
| 19     | Power control for channel 4
| 20     | LED indicator for trigger 1. Lights up when triggered
| 21     | LED indicator for trigger 2. Lights up when triggered
| 22     | LED indicator for channel 1. Green = Internal channel 1 selected, but no output. Yellow = Shock collar 1 selected - see config section - but no output (2.5mm CHAN1 unused). Red = output on (either 2.5mm output or shock collar, depending on config)
| 23     | As above, but for channel 2 
| 24     | As above, but for channel 3
| 25     | As above, but for channel 4

## Screen
![screen layout]

| Number | Description
| ------ | -----------
| 1      | Softbutton - Text for softbutton 1
| 2      | Softbutton - Text for softbutton 2 
| 3      | Softbutton - Text for softbutton 3
| 4      | Softbutton - Text for softbutton 4
| 5      | Menu items. White text is the currently selected option
| 6      | Option corresponding to selected menu item. Can be changed using the Adjust dial
| 7      | Current power level for each channel. The blue bar represents the current maximum power the pattern can use, as set by the power control dials. The yellow bar inside it shows the current power level the patten is using. For most patterns, these will be the same, but some (e.g. triggered climb) will have a much smaller (but rising) yellow bar.

## On power up
On first power up, the box shows a menu of available patterns. Either the highlighted pattern can be started by pressing soft button 1, or the config menu can be entered by using soft button 2.

## Config
Configuration options so far:
* Channel config - For each channel 1-4, picks if either the internal output (on the 2.5mm connector) should be used, or if a shock collar should be triggered

* Collar config - Allows 4x shock collars to be configured. Each shock collar needs to be paired to the box, and the bottom option allows for testing the collar with the current settings. The "Chan." option corresponds to the CH button on the original remote, and it probably makes sense to be left as 1 for all collars, as each will have a unique ID anyway. Note that the mode (shock / vibrate / beep) needs to be set here, and won't change outside of this config screen (for the time being, at least)

* LED brightness - Pretty much what is says. If you value your eyesight, single digit values are good.

* Ramp up time - When starting a patten, how long it takes to ramp up to the power level set for the channel on the front panel

## Shock collars
If the box is fitted with a 433Mhz AM transmitter in J11 on the main board, it can be used to control one specific type of shock collar:

![Shock collar]

(note that there is 915MHz variant of these that looks identical on the outside...)

These will only function in certain patterns which have simpler output demands, and is generally pretty experimental.



[device layout]: images/layout.png "Device layout"
[screen layout]: images/screen.png "screen layout"
[shock collar]: images/shock_collar.jpg "Supported shock collar"


