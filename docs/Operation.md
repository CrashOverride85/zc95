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
| 6      | Power in. MKI: Should be centre positive 15v DC. MKII: any USB-C charger
| 7      | MKI: Either Serial port, or if optional audio input board fitted, can be toggled between serial and audio input. MKII: separate serial and audio sockets (not in photo). Both: In serial mode, Tip is TX / output from box, Sleeve is ground.
| 8      | Accessory port. Includes power, serial and 3x GPIO pins. See "Accessory port" in misc section. Currently not really used by much.
| 9      | Trigger input 1. Shorting either Sleeve or Ring to Tip registers trigger. The box can differentiate between S-T and R-T triggers (i.e. each trigger socket can react to two different inputs)
| 10     | Trigger input 2. As above, except currently unused.
| 11     | 2.5mm (MKI) or 3.5mm (MKII) socket for channel 1 output
| 12     | 2.5mm (MKI) or 3.5mm (MKII) socket for channel 2 output
| 13     | 2.5mm (MKI) or 3.5mm (MKII) socket for channel 3 output
| 14     | 2.5mm (MKI) or 3.5mm (MKII) socket for channel 4 output
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

* Remote access - Allows for remote access to the ZC95 over WiFi + BLE (if running on a Pico-W) and serial, for the purpose of either remotely controlling it or uploading Lua scripts. See [Remote Access](./RemoteAccess.md) for serial & WiFi, and [BLE Streaming](./BleStreaming.md) for BLE remote control

* BLE peripherals - Only visible if running on a Pico-W. For configuring bluetooth remotes; see [Bluetooth peripherals page](./BluetoothPeripherals.md)

* Output - Settings related to signal output. Options:
  - Power level - High (default), Medium or Low
  - Start ramp duration - When starting a pattern, how many seconds does it take to ramp up to the power level set for the channel on the front panel
  - Extended ramp conf. - in addition to the initial ramp up period whenever a pattern is started, the ZC95 can also gradually increase the power level over an extended period of time - from a few minutes to over an hour. The extended ramp configuration menu has these options:

    * Show ramp start - Yes or No. If Yes, when running a pattern the top item on the list will always be "Ramp start". To start the ramp up, select this option, and press the top left soft button (labelled "Start"). 
    * Start level - 1% - 100%. At what percentage of the power selected on the front panel does the ramp start. Setting to 100% effectively disables the ramp function
    * Time per p.p. - Time taken in seconds to increase the power level by one percentage point. This increase is relatively smooth, and will increase the power level in 0.1% increments to achieve the selected rate.

    With either "Start level" or "Time per p.p." selected, the bottom of the menu will indicate the estimated time in minutes for the ramp to complete.

* Channel config - For each channel 1-4, picks if either the internal output (on the 3.5mm connector) should be used, or if a shock collar should be triggered

* Collar config - Allows 4x shock collars to be configured. Each shock collar needs to be paired to the box, and the bottom option allows for testing the collar with the current settings. The "Chan." option corresponds to the CH button on the original remote, and it probably makes sense to be left as 1 for all collars, as each will have a unique ID anyway. Note that the mode (shock / vibrate / beep) needs to be set here, and won't change outside of this config screen (for the time being, at least)

* Display options - sub-menu, with:
  - LED brightness - Pretty much what it says. If you value your eyesight, single digit values are good.

  - Power level display - where to show the numeric power level as percentage. Options are:
    - Off (default) - Only show power level as a bar graph
    - Disappearing - Show in large text on screen for a few moments on change
    - In bar graph - Show value, rotated 90 degrees, inside the corresponding bar graph
    - Both - Enable Disappearing text display and in bar graph

  - Button brightness - controls how bright the LEDs in the 4 front panel buttons are

  - MKII only: Display brightness - as you would expect, changes the display brightness

  - MKII only: Status bar text. What the status bar should show. Options:
    - Running pattern - name of the currently running pattern, or blank if nothing running. This is what MKI's show
    - Battery current - current draw from battery in mA. -ve indicates current being drawn from battery, +ve current going into battery (i.e. charging).
      For PCB versions >= 2.3, this may go negative even when plugged in, as if the charger can't supply enough current, the difference will come from the battery.

* Audio input - Displayed if audio board present by default, depends on "Hardware config > Audio" setting. See [Audio Input](./AudioInput-Operation.md)

* Hardware config - configure various hardware settings. Shouldn't need changing. Options:
  - Audio - control display of audio options. See [Audio Input](./AudioInput-Operation.md) 
  - Debug output - where debugging information is sent:
    - Accessory port
    - Aux port
    - Off

    Note that if the Aux port is configured to be used for audio, picking Aux port here is essentially the same as "Off". Also worth noting that when the box is first powered up, debug info is always sent to the accessory port until the configuration is read, before potentially switching to aux or off.

  - MKI only: Aux port use - what the Aux port on the front is used for. Options:
    - Audio input
    - Serial I/O

    If the audio board isn't present, this menu has no effect, i.e. Aux is always in Serial I/O mode.

  - v2.3+ only: Batt charge current. Set the maximum charge current for the battery between 0 mA and 3000 mA.

      **Caution** : Do not rely on this - the battery must be capable of being _safely_ charged at 2000 mA, even if a lower value is set for battery longevity / power consumption (whatever) reasons. 2000 mA is the default for the charge controller, so it's possible there are some scenarios where the default may get used.

* Battery info - MKI & MKII: Shows battery voltage and state of charge (%). MKII only: Also shows estimated battery capacity and capacity remaining in mAh along with current flow in/out of battery in mA (-ve is out of battery, +ve is in, i.e. charging).

* About - shows firmware version of main board, and zc624 output board


## Shock collars
If the box is fitted with a 433Mhz AM transmitter in J11 on the main board, it can be used to control one specific type of shock collar:

![Shock collar]

(note that there is 915MHz variant of these that looks identical on the outside...)

These will only function in certain patterns which have simpler output demands, and is generally pretty experimental.



[device layout]: images/layout.png "Device layout"
[screen layout]: images/screen.png "screen layout"
[shock collar]: images/shock_collar.jpg "Supported shock collar"
