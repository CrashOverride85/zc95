# Lua

## Capability
Lua scripts can be written / uploaded (see [Remote Access](./RemoteAccess.md) for how to upload) to add new patterns to the ZC95. These scripts are able switch each channel on/off, set the frequency and pulse width, and set the power level (scaled to what the front panel is set to - i.e. a Lua script can't set the output power to higher than set on the front panel).
They can also receive notification of settings being changed via the menu, and inputs from the external trigger inputs along with the top left soft button being pressed.
The most significant thing they can't do (that inbuilt patterns can) right now is anything to do with audio.

## Example scripts
There are a few example scripts in `remote_access/lua`, which demonstrate some of things that can be done from Lua. These scripts are:

* fire.lua - If the soft button is pressed, all channel's are activated for as long as the button is held down. If any of the 4 external triggers (assuming stereo connectors are being used), the the correspond channel (1-4) is activated for as long the trigger is active.

* toggle.lua - switches between channel 1+2 & 3+4 at a speed that can be set via the menu. If the channels are switched on constant or just pulsed can also be set from the menu

* waves.lua - a waves pattern that is slightly more advanced than the inbuilt one. For each channel, this varies the frequency between 25Hz and 250Hz and the pulse width between 40us and 200us, with the time taken for each cycle being set per channel on the menu.

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
            choices = {
                {choice_id = 1, description = "Pulse"},
                {choice_id = 2, description = "Constant"}
            }
         }
    }
}
```
`name = "Toggle"` sets the name for the script - this is prefixed with `U:` then used on the patterns menu.

A menu entry is displayed when the script is running for each item in `menu_items`; each must be given a unique id, numbered sequentially from 1. There are two types supported:
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
* `zc.ChannelPulseMs(channel, duration)` - will pulses a channel on for the specified number of milliseconds
* `zc.ChannelOn(channel)` - switches a channel on until switched off
* `zc.ChannelOff(channel)` - switches a channel off

See zc.* functions section below for more details along with all available zc.functions

## zc.* functions


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
    * positive pulse width (1-255) us
    * negative pulse width (1-255) us
```
Sets the pulse width used for the channel, defaults to 150us, but this default may move into the config menus at some point.
For symmetric pulses (as used by most/all inbuilt patterns), these two values should be the same. 

### ChannelPulseMs

### ChannelOn


### ChannelOff










