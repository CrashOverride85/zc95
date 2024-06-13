# Bluetooth Peripherals

The ZC95 allows for the use of small shutter remotes meant for phones, and custom Bluetooth (HID) devices. The remotes can be used to either simulate keypresses / triggers, or passed through more directly to a Lua script to use. Events from custom Bluetooth devices can only be used with Lua scripts.

When bluetooth is active, a logo is displayed in the bottom right corner of the screen. White means trying to connect and blue connected. 

Note that most of these bluetooth remotes will disconnect after a few minutes of inactivity. On pressing a button, most reconnect fairly quick (in a second or two), but that keypress is missed.

## Limitations
* Only supports (some) BLE remotes, not bluetooth classic. Other BLE device types may pair successfully, but are very unlikely to work
* Does not work in Audio modes
* Does not work if a pattern is being ran remotely over WiFi (but does over serial)
* Only one device can be paired at once

## Tested remotes
| #   | Photo |  Name       | Works?  |  Comments 
|---  |---    |---          |---      |---                |
| 1   |![r1]  | BLE-M3      | Yes     |  All buttons work 
| 2   |![r2]  |             | No      |  Bluetooth classic 
| 3   |![r3]  | DY Shutter  | Yes     |  D pad works well. Center button mostly works, bottom button doesn't always work on first press?
| 4   |![r4]  | BLE-M3      | Yes     |  All buttons work, but center and bottom button are read as the same
| 5   |![r5]  | AB Shutter3 | Yes     |  Works well - lowest latency of all remotes. But both buttons do the same
| 6   |![r6]  | ZL-03       | Sort of |  Can be awkward to pair. Significant delay between pressing button and it registering. Can double tap up/down buttons for left/right 
| 7   |![r7]  | Yiser-Y6    | Sort of |  Only able to pair if the BTStack library is patched, see [build from source notes](./SourceBuildNotes.md)  (release version **is** patched). Centre button and "2" button do the same

## Configuration
The bluetooth configuration menu is in `Config -> BLE peripherals`. This config menu is hidden if running on a Pico non-W.

Options:
### Enabled
If "On", the box will try to connect to the paired bluetooth device whenever a pattern is started (excluding audio patterns).

### Scan/Pair
When selected, will start a scan and list all devices found. If the currently paired device is found, it is prefixed with a `*`.

Press the top left "Pair" button to try and pair with the selected device. All going well you should get a "Success" message after a couple of seconds or so.
Only one device can be paired at once, and successfully pairing with a new device causes the previous one to be mostly forgotten about.

### Test
Connects to the paired bluetooth remote, and reports how any keypresses received are being interpreted on screen. Currently the only keys it can recognise are:
- KEY_UP
- KEY_DOWN
- KEY_LEFT
- KEY_RIGHT
- KEY_BUTTON
- KEY_SHUTTER
- KEY_UNKNOWN

If this screen shows "Status: Connected", but pressing buttons on the remote does nothing, it's (currently) not a compatible remote.

### Configure remote
This screen is used to map a received keypress to an action. Note that bluetooth is only active when a (non-audio) pattern is running, i.e. not on the pattern selection menu.

Each of the received key press types can be set to one of:

- Soft button actions:
    - `Top left soft` 
    - `Bottom left soft` - Back button. Will exit pattern resulting in bluetooth disconnecting
    - `Top right soft` - Up for patterns with multiple options
    - `Bottom right soft` - Down for patterns with multiple options

- Adjust / rotary dial actions:
    - `Adjust left`
    - `Adjust right`

- Trigger action
    - `Trigger1-A` - equivalent to Tip->Sleeve short on Trigger1 socket
    - `Trigger1-B` - equivalent to Tip->Ring on short Trigger1 socket
    - `Trigger2-A` - equivalent to Tip->Sleeve on short Trigger2 socket
    - `Trigger2-B` - equivalent to Tip->Ring on short Trigger2 socket


**Note:** For Lua scripts with `bluetooth_remote_passthrough = true` in the `Config` section, these mappings have no effect.

### Misc
A fairly obvious pattern to use a bluetooth remote with is "Fire", however if using this you'll need to set its "Mode" option to "Pulse", as "Continuous" only works if the button is held down, which can't be detected from a bluetooth shutter button.

## Custom Bluetooth devices
The ZC95 can be used to connect to and receive events from custom bluetooth devices. For an example device than can be used to control the power level of the ZC95 over bluetooth, see [this example](../misc/Bluetooth/HidExample/).

[r1]: images/bt_remotes/1.jpg "BT remote 1"
[r2]: images/bt_remotes/2.jpg "BT remote 2"
[r3]: images/bt_remotes/3.jpg "BT remote 3"
[r4]: images/bt_remotes/4.jpg "BT remote 4"
[r5]: images/bt_remotes/5.jpg "BT remote 5"
[r6]: images/bt_remotes/6.jpg "BT remote 6"
[r7]: images/bt_remotes/7.jpg "BT remote 7"
