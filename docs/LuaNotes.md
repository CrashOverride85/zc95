# Lua

## Capability
Lua scripts can be written / uploaded (see [Remote Access](./RemoteAccess.md) for how to upload) to add new patterns to the ZC95. These scripts are able to switch each channel on/off, set the frequency and pulse width, and set the power level (scaled to what the front panel is set to - i.e. a Lua script can't set the output power to higher than set on the front panel).
They can also receive notification of settings being changed via the menu, and inputs from the external trigger inputs along with the top left soft button being pressed.
The most significant thing they can't do (that inbuilt patterns can) right now is anything to do with audio.

## Example scripts
There are a few example scripts in `remote_access/lua`, which demonstrate some of things that can be done from Lua. These scripts are:

* fire.lua - If the soft button is pressed, all channel's are activated for as long as the button is held down. If any of the 4 external triggers (assuming stereo connectors are being used), the the corresponding channel (1-4) is activated for as long the trigger is active.

* toggle.lua - switches between channel 1+2 & 3+4 at a speed that can be set via the menu. If the channels are switched on constant or just pulsed can also be set from the menu

* waves.lua - a basic waves pattern. For each channel, this varies the frequency between 25Hz and 250Hz and the pulse width between 40us and 200us, with the time taken for each cycle being set per channel on the menu.

* bluetooth_fire.lua - uses Left/Up/Down/Right buttons on a bluetooth remote to trigger chanel's 1 to 4 respectively for a configurable pulse duration

* acc_test.lua - uses the accessory port to cycle between HIGH/LOW on the 3 output lines. See "AccIoWrite" section below for more details. Doesn't produce any estim output, so not really of any practical use, just a demo of how to use the accessory port.

The inbuilt Waves, Climb & Orgasm patterns are also written in Lua (contributed by someone who prefers to remain anonymous); the source for these can be found in `source/zc95/LuaScripts/`. Note that these are designed to closely resemble the patterns of the same name from another popular box, so might not be the best example to follow for new scripts.

## toggle.lua

It's probably easier to explain Lua as implemented in the ZC95 by going though one of the example scripts.

toggle.lua:
```
_delay_ms = 500

Config = {
    name = "Toggle",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Delay",
            group = 0,
            id = 1,
            min = 100,
            max = 2000,
            increment_step = 100,
            uom = "ms",
            default = _delay_ms
         },
         {
            type = "MULTI_CHOICE",
            title = "Output",
            group = 0,
            id = 2,
            choices = {
                {choice_id = 1, description = "Pulse"},
                {choice_id = 2, description = "Constant"}
            }
         }
    }
}

_channel12_on = false
_wait_until_ms = 0
_pulse_mode = true

function MinMaxChange(menu_id, min_max_val)
    if (menu_id == 1)
    then
        _delay_ms = min_max_val
    end
end

function MultiChoiceChange(menu_id, choice_id)
    if (menu_id == 2 and choice_id == 1)
    then
        _pulse_mode = true
    end

    if (menu_id == 2 and choice_id == 2)
    then
        _pulse_mode = false
    end
end

function Loop(time_ms)
    if (time_ms > _wait_until_ms)
    then
        ToggleChannel();
        _wait_until_ms = time_ms + _delay_ms
    end
end

function ToggleChannel()
    if (_channel12_on == true)
    then
        zc.ChannelOff(1)
        zc.ChannelOff(2)

        if (_pulse_mode == true)
        then
            zc.ChannelPulseMs(3, 100)
            zc.ChannelPulseMs(4, 100)
        else
            zc.ChannelOn(3)
            zc.ChannelOn(4)
        end
        _channel12_on = false
    else
        if (_pulse_mode == true)
        then
            zc.ChannelPulseMs(1, 100)
            zc.ChannelPulseMs(2, 100)
        else
            zc.ChannelOn(1)
            zc.ChannelOn(2)
        end

        zc.ChannelOff(3)
        zc.ChannelOff(4)
        _channel12_on = true
    end
end
```

The `Config` section is used to set the name and build the on screen menu:
```
Config = {
    name = "Toggle",
    menu_items = {
        {
            type = "MIN_MAX",
            title = "Delay",
            id = 1,
            group = 0,
            min = 100,
            max = 2000,
            increment_step = 100,
            uom = "ms",
            default = _delay_ms
         },
         {
            type = "MULTI_CHOICE",
            title = "Output",
            id = 2,
            group = 0,
            choices = {
                {choice_id = 1, description = "Pulse"},
                {choice_id = 2, description = "Constant"}
            }
         }
    }
}
```
`name = "Toggle"` sets the name for the script - this is prefixed with `U:` then used on the patterns menu.

A menu entry is displayed when the script is running for each item in `menu_items`; each must be given a unique id, numbered sequentially from 1.
 _Optionally_, each item can be given a `group` number - this only has any affect when ran remotely using the GUI, and allows related options to be group together, instead of appearing in one long list (useful for scripts with many options).

 There are two types supported:
* `MIN_MAX` - shows a horizontal bar graph that can be changed between the set min/max using the adjust dial. The unit of measure (uom) text is displayed as suffix to the numeric value in the bar chart 
* `MULTI_CHOICE` - used to show a menu option that allows for one of multiple settings to be picked. Each choice must have a unique id.

```
function MinMaxChange(menu_id, min_max_val)
    if (menu_id == 1)
    then
        _delay_ms = min_max_val
    end
end
```
If present, the `MinMaxChange(menu_id, min_max_val)` function is called whenever a `MIN_MAX` menu type is changed, with the ID of the menu and the new value.

```
function MultiChoiceChange(menu_id, choice_id)
    if (menu_id == 2 and choice_id == 1)
    then
        _pulse_mode = true
    end

    if (menu_id == 2 and choice_id == 2)
    then
        _pulse_mode = false
    end
end
```
Similar to MinMaxChange but for `MULTI_CHOICE` menu types; if present, the `MultiChoiceChange(menu_id, choice_id)` function is called whenever a `MULTI_CHOICE` type menu entry is changed, with the menu ID and the choice ID now selected.

```
function Loop(time_ms)
    if (time_ms > _wait_until_ms)
    then
        ToggleChannel();
        _wait_until_ms = time_ms + _delay_ms
    end
end
```
The `Loop(time_ms)` function is mandatory, and is called periodically for as long as the Lua script is running. The time_ms parameter is how long, in milliseconds, since the box was powered up.


```
function ToggleChannel()
    if (_channel12_on == true)
    then
        zc.ChannelOff(1)
        zc.ChannelOff(2)

        if (_pulse_mode == true)
        then
            zc.ChannelPulseMs(3, 100)
            zc.ChannelPulseMs(4, 100)
        else
            zc.ChannelOn(3)
            zc.ChannelOn(4)
        end
        _channel12_on = false
    else
        if (_pulse_mode == true)
        then
            zc.ChannelPulseMs(1, 100)
            zc.ChannelPulseMs(2, 100)
        else
            zc.ChannelOn(1)
            zc.ChannelOn(2)
        end

        zc.ChannelOff(3)
        zc.ChannelOff(4)
        _channel12_on = true
    end
end
```

The `ToggleChannel()` function (could have been named anything) is switching between the two pairs of channels. The relevant parts are the zc.* functions:
* `zc.ChannelPulseMs(channel, duration)` - will pulse a channel on for the specified number of milliseconds
* `zc.ChannelOn(channel)` - switches a channel on until switched off
* `zc.ChannelOff(channel)` - switches a channel off

See zc.* functions section below for more details along with all available zc.functions

## zc.* functions
Functions that can be called from Lua scripts to control the box. In addition to these, `print("<whatever>")` can be used from scripts (no `zc.` prefix); the output will appear in the serial output prefixed with '`[LUA]`', and in the debug window of the `pattern_gui.py` GUI if running remotely. 

### SetPower
```
Params:
    * channel - 1-4
    * power   - 0-1000
```
Sets the output power of channel from 0 to 1000. This output power is scaled based on what the front panel dial is set to for the channel. E.g if the front panel is set to 50% and a power level of 500 is set, the result will be a power level of 250 (25%).

If there is no `Setup()` function in the script, all channels will default to full power (i.e. only affected by front panel dials), so it often isn't necessary to use this. Most inbuilt patterns don't change the power level.

### SetFrequency
```
Params:
    * channel number (1-4)
    * frequency (1 - 300) Hz
```
Sets the output frequency of the specified channel. Currently defaults to 150Hz, but this default may move into the config menus at some point.

### SetPulseWidth
```
Params:
    * channel number (1-4)
    * positive pulse width (0-255) us
    * negative pulse width (0-255) us
```
Sets the pulse width used for the channel, defaults to 150us, but this default may move into the config menus at some point.
For symmetric pulses (as used by most/all inbuilt patterns), these two values should be the same. 

### ChannelPulseMs
```
Params:
    * channel number (1-4)
    * duration (ms)
```
Switch the channel on for the specified number of milliseconds, using the previously set frequency, pulse width and power level (or the defaults, if not changed).


### ChannelOn
```
Params:
    * channel number (1-4)
```
Switch on the specified channel, until ChannelOff is called, using the previously set frequency, pulse width and power level (or the defaults, if not changed).

### ChannelOff
```
Params:
    * channel number (1-4)
```
Switch off the specified channel.

### AccIoWrite
```
Params:
    * Accessory I/O line number (1-3)
    * State: true (high) or false (low)
```
Controls the 3 I/O lines on the accessory port - allows setting between high (3.3v) and low.

Note that the default state of these 3 lines from power on is HIGH, so bear that in mind when connecting anything.
These lines can only source a few milliamps safely, so should only be used for signalling, i.e. it's probably best to connect a logic level MOSFET to switch anything more substantial than an LED.

Also worth noting that there is _very_ limited protection on this port, something to be corrected in a possible future hardware revision, so be careful to avoid higher voltages. In particular, there is 12v on pin 7 (in hindsight a poor decision) - connecting this to pretty much any other pin would be bad.

Minimal example for using the accessory port to control 3 LEDs. Can be used with the "acc_test.lua" example LUA script to switch between each LED in turn.

![acc port]


## Special functions
These are functions that will be automatically called when applicable whilst the Lua script is running. With the exception of `Loop()`, all are optional. 

### MinMaxChange(menu_id, new_value)
Called when a `MIN_MAX` type pattern option is changed, and is called with the menu ID of the option, and the new value (which should lie between the min and max configured).

### MultiChoiceChange(menu_id, choice_id)
Called when a `MULTI_CHOICE` type pattern option is changed, and is called with the menu ID of the option, and the ID of the selected choice.

### SoftButton(pushed)
Called with `pushed=True` when the top left soft button is pressed, and then again when it is released with `pushed=False`. The soft button text is set by specifying `soft_button = "<label>"` in the `Config = {}` section. See `fire.lua` script for an example.

### ExternalTrigger(socket, part, active)
Called when an external trigger happens.

Socket: can be either "`TRIGGER1`" or "`TRIGGER2`" for the Trigger1 and Trigger2 sockets respectively. 

Part: can be either "`A`" or "`B`". With a stereo 3.5mm TRS cable inserted, shorting Tip and Sleeve is part `A` (trigger LED lights up green). Shorting Tip and Ring is part `B` (trigger LED lights up red). When triggered, `active` will be `True`, when released it will be `False`.

### BluetoothRemoteKeypress (key)
See bluetooth_fire.lua for an example of this.

If `bluetooth_remote_passthrough = true` is present in the config section, key presses from a [connected bluetooth remote](./Bluetooth.md) are passed through to this function, instead of using the mappings configured in the bluetooth menu. Key will be one of:
* "`KEY_BUTTON`"
* "`KEY_UP`"
* "`KEY_DOWN`"
* "`KEY_LEFT`"
* "`KEY_RIGHT`"
* "`KEY_SHUTTER`"
* "`KEY_UNKNOWN`"

Note that unlike `SoftButton` and `ExternalTrigger`, there is only a single event, i.e. not pressed followed by released.

If `bluetooth_remote_passthrough = false` (or is absent), this function is never called, and key presses from a connected bluetooth remote are interpreted according to the configured mappings. E.g. if SHUTTER is mapped to "`Top left soft`", pressing the shutter button will cause `SoftButton(true)` immediately followed by `SoftButton(false)` to be called.

### BluetoothHidEvent (usage_page, usage, value)
Allows the ZC95 to receive events from custom bluetooth devices. See `bluetooth_hid.lua` and the example BT project [HidExample](../misc/Bluetooth/HidExample/).

When paired to bluetooth HID device, this method will be called for each event received. If you value your sanity, I would suggest not attempting to write Lua scripts to support miscellaneous bluetooth devices unless you're particular familiar with bt (I'm not) and _exactly_ what the device in question is sending. 

### Setup()
Called once when the pattern is started, before `Loop()`. It can be used to do any initial setup, including setting power level. If this function does _not_ exist, the power level is defaulted to 1000, otherwise it is set to 0 and can be set to something more appropriate here. 

### Loop(time_ms)
Called periodically for as long as the pattern is running. `time_ms` is how long in milliseconds the box has been powered on, and since v1.7 is a floating point number with microsecond precision. 

`Loop()` is called as often as possible by default, but how often will depend enormously on how much work is done in the Loop() function - expect an empty Loop() to be called around every 50us, and a complex loop() to be called closer to every 4000us (or more).

If a specific frequency is required (rather than just as often as possible), a `loop_freq_hz = n` option can be added to the `Config = {}` block, where `n` is between 1 and 400. Be aware that for particularly complex scripts, higher values are unlikely to work well - and note that this setting will only reduce how often Loop() is called when compared to the default.

[acc port]: images/lua_acc_port.png "Accessory port"
