# BLE remote access
## Summary
Right now, the BLE remote access functionality is likely only of use to developers. It allows the box to be controlled at a more fundamental level remotely, where patterns are generated remotely and streamed to it. 

There are two main ways in can be used:
1. For each channel, the pulse width, frequency and power level can be set. When the channel is then switched on, it will continue outputting with these settings until either changed, or BLE disconnection. This level of control is all most inbuilt patterns use (arguably more, as most inbuilt patterns don't change the power level).
There's a pattern_runner python script, and a couple of python patterns that demonstrate this.
2. Individual pulse pairs can be streamed to the box. There's some examples of doing this from a CSV file. This allows for the possibility of creating triphase effects if enabled, if the pulses are created with that in mind. This mode should be considered experimental. 

Whilst the examples are in python, it should be easy enough to use this from embedded projects.

Unless you _know_ you need #2, I'd suggest #1 is far easier to use and produce good results with. 
Whilst there's nothing stopping you, mixing the two approaches on the same channel is unlikely to go well.

To use either mode, start BLE remote access by going to `Config -> Remote access -> BLE remote access`.

All python scripts are in [misc/PythonBle](../misc/PythonBle/).

## Configuration
BLE remote access can be configured from `Config -> Remote access -> BLE config`. There are two options:
* Allow triphase - `Yes`/`No`. See "Channel isolation & triphase" section later, but allows for channel isolation to be remotely disabled allowing for tripahse affects (**a triphase cable is still required**)
* Power dial mode - affects how the front panel dials impact the power level output
  - `Limit` - Sets the maximum output power. E.g. if a power level of 500 is set remotely and the dial is at 50%, you get 50% power. If 750 is set remotely, you still get 50%
  - `Scale` - the output power is scaled according to the power dial setting. E.g. if a power level of 500 is set remotely and the dial is at 50%, you get 25% power. If 750 is set remotely, you get 37.5%

The intention is for the `Scale` option to be used when a pattern is to be generated remotely, but using the power dials on the ZC95 to control output power as needed, and for the `Limit` mode to be used where ZC95 is being fully controlled remotely, but keeping the the ability to set a limit for each channel to avoid unexpectedly high outputs.

## Setting pulse width, frequency and power level 
All relevant characteristics are grouped together in the custom "General control service", one per property, see table under "Services & characteristics".

These are hopefully fairly self explanatory; e.g. the general approach to output a 100hz, 120us signal on channel 1 and 50% power would be:
- set "Channel 1 pulse width" to 120
- set "Channel 1 frequency" to 100
- set "Channel 1 power level" to 500 (range is 0-1000 for 0-100%)
- Switch on power for the channel by setting "Channel 1 power enable" to 1

Changing the pulse width/frequency/power level whilst the channel is on _does_ work, and is how internal patterns work.

The difference between 0% output and a switched off channel is how it shows on the user interface. Switching a channel off sets the LED to green, and on to red. Setting the power to 0% changes the yellow bar in the power graph to a stub at the bottom, setting to 100% causes it to match the height of the blue bar.

### Examples

The `pattern_runner.py` script should scan for the ZC95, connect, then start the specified python pattern.

The script expects 1 parameter, which is the pattern to run, expected to be in the `patterns/` directory. Currently there are two:
- `intense.py` 
- `toggle.py`

E.g. `pattern_runner.py toggle`


## Sending individual pulses
Using this mode requires a slightly greater understanding of exactly what signals the zc95 can generates, and, fair warning, is just much harder to use. 

### ZC95 output capability
The ZC95 has been designed around the idea of generating pulses in pairs - I've called them positive and negative, but in reality A and B may have made more sense. I'm sticking with it for now.
E.g. one pulse pair where both positive and negative pulses are 80us looks something like:

![p1]

And a pulse pair where the positive pulse was 80us and the negative pulse 0us would be:

![p2]

To generate a 150hz output similar to the below it would be necessary to send pulse pairs to the ZC95 with timestamps 10ms apart:

![p150hz]

(pulse widths not to scale!)

Firmware limitations of the ZC95 (which I currently have no intention to change) mean that it cannot:
* Swap the positive/negative pules around
* Insert a short - less than around 500us - delay between the positive and negative pulses. For longer, you can send a pulse pair with only a negative pulse then a pair with only a positive pulse

### Channel isolation & triphase
By default, the ZC95 will never generate more than one pulse at the same time - there will always be a minimum of 150us between a pulse pair finishing and the next pulse pair being generated; this is mostly for safety reasons to avoid unanticipated interactions between the channels. Where pulses have the same timestamp (or the same message is used to generate pulses on multiple channels), these pulses will automatically be separated by the ZC624 output board when channel isolation is enabled (default).

However, this prevents the generation of triphase effects where multiple channels need to be active at once, so it can be disabled. Whilst the channel outputs still need to be physically connected with a suitable triphase cable to generate triphase effects, **do not place any electrodes above the waist** with channel isolation disabled.

Steps required to support triphase effects:
* Use a triphase cable to connect two channels together
* Using the `Config -> Remote access -> BLE config` screen, set `Allow triphase` to `Yes`
* Write 0x00 to the the "Channel isolation" characteristic
* -> The ZC95 screen should show "Triphase: ACTIVE"


### Message format
When in this mode the ZC95 is expecting to receive messages which have:
* time_us - time to generate the pulse. To allow for buffering, this should be in the region of 10-100ms in the future, and not more than 1 second in the future
* positive & negative pulse width
* channel mask - which channel(s) to generate the pulse on
* power - what power level to set

A single BLE packet can have multiple messages, and these messages are streamed over the "Pulse stream" service, using the "Pulse stream" characteristic.

Messages should have the format:

```
 0                8                16               24               32  
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | cmd_type       | pulse_width_pos| pulse_width_neg| channel_mask   |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                              time_us                              |
 |                                                                   |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | Amplitude                       | Reserved                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ```

 Or as a C structure:
 ```
 struct ble_message_pulse_t
{
    uint8_t cmd_type;          
    uint8_t pulse_width_pos;   
    uint8_t pulse_width_neg;   
    uint8_t channel_mask;  
    uint64_t time_us;
    uint16_t amplitude;        
    uint16_t reserved;
} __attribute__((packed));
```

(fields are little endian where applicable)

`cmd_type` can be:
* 0x00 = START
* 0x01 = PULSE

`pulse_width_pos` & `pulse_width_neg` - pulse widths in us. All values supported

`channel_mask` - which channel(s) the pulse is for, and if positive, negative or both pulses should be generated for that channel:

```
 0         2         4         6         8
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |C1+ |C1- |C2+ |C2- |C3+ |C3- |C4+ |C4- |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ```

So, for example, 0x03 would generate the pulse pair on just channel 1, 0x0C on channel 2 etc.

`time_us` - Time the pulse should be generated. `time_us = 0` is when the START message was sent, so all times are expected to be relative to that message, and must be sequential

`amplitude` - Power level - 0-1000 for 0-100%. **Warning**: At present power level changes happen asynchronously to pulse generation, so while this will change the power level, it may not necessarily be in time for the pulse. If you're wanting to modulate the power on a per-pulse basis, consider sending a message with the new power level, and a 0us pulse width, with a timestamp a few hundred microseconds before the new power level is needed. 

`reserved` - Reserved for future use. Send as 0x0000.

### Packet format
Each packet has a 4 byte header, followed by up to 14 pulse messages as described previously, giving a total packet size of 14 x 16 + 4 = 228 bytes, so this is assuming DLE is supported.
As the total size of 1 packet with 1 message is 20 bytes, that should be an option if DLE is not available.

Packets should have the format:

```
 0                8                16               24               32  
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | message_count  | counter        | reserved                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                                   |
 |                              message 1                            |
 |                                                                   |
 |                                                                   |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                                                                   |
 |                              message n...                         |
 |                                                                   |
 |                                                                   |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ```
Fields:

* `message_count` - the number of messages included in the packet. The zc95 verifies that the total packet size is `message_count * 16 + 4`, and any packets where this isn't true are discarded

* `counter` - increment for each packet sent; wrap around is expected. Only used for debug output, so if set to 0 for all messages, the debug output from the ZC95 will give an incorrect indication of lost packets, but everything will otherwise work.

* `reserved` - reserved for future use, send as 0x0000

### Sending pulses
The general approach is:
* Connect
* _Optionally_ enable triphase support 
* Send a start message
* Send pulses for as long as you want output
* Disconnect

The start message is a packet with a single message with `cmd_type` = `0x00`, and all other values 0 too. All pulse messages sent are expected to a have a time_us relative to the time the start message was sent.

### Other notes
#### Buffer size
The ZC95 can buffer up to 50 pulses per channel (so 200 total). Note that if a message is received with a channel mask of 0xFF (i.e. affects all channels), this would use up one entry in each queue.

#### Debug output
When streaming pulses to the ZC95, it's debug output will show messages approximately once per second similar to:

```Packet count: 21, Message count: 147, Missing packets: 0, fifo full: 0, msg_old: 0, msg_past: 0, msg_future: 0```

All the fields are counts since the previous message, and are:
* `Packet count` - number of packet received
* `Message count` - number of messages received
* `Missing packets` - using the `counter` field in the packet, the number of packets that appear to be missing
* `fifo full` - number of messages dropped due to the FIFO queue for the channel being full, i.e. the limit of 50 queued pulses for the channel has been reached
* `msg_old` - number of pulses dropped due to being too far past the time_us by the time they were processed. Shouldn't happen unless messages are being sent with a time_us only a few microseconds in the future
* `msg_past` - number of messages dropped due to the time_us being in the past
* `msg_future` - number of message dropped due to time_us being more than 1 second in the future

### Python example
At present there is only one example python script using this mode: `zc95_csv_stream.py`. As the name suggests, this takes pulses from a CSV file and streams them to the ZC95. Once the end of the CSV is reached, it starts back at the top but increments the time_us sent on each loop through so the ZC95 sees a sequential time_us.

Example file:
```
time_us,channel,pos_width_us,neg_width_us,power
 10000, 2, 80, 80, 1000
 20000, 1, 80, 80, 1000
 20000, 2, 80, 80, 1000
 ```

This would generate a 50hz signal on channel 1, and a 100hz signal on channel 2, both at 80us and full power.

There is a second python script - `csv_convert.py` - which can used to convert CSV files from the below format to something `zc95_csv_stream.py` can read and stream:
```
"Stage","SeqNr","Timestamp","Phase","Width","Vprim"
A,1533,26646450,0,144,2808
A,1535,26681260,0,143,2801
```
See the [patterns312](https://github.com/Onwrikbaar/NeoDK/tree/main/patterns312) directory in [@Onwrikbaar's](https://github.com/Onwrikbaar) [NeoDK](https://github.com/Onwrikbaar/NeoDK) repository for examples. It works with the [toggle](https://github.com/Onwrikbaar/NeoDK/blob/2d3f97142200198a6b14f82a4e323c43dab2d1db/patterns312/Toggle.csv) pattern - it might work with others. YMMV. 

## Services & characteristics
|  |  |  |  |  |  |
|----|----|----|----|----|----|
| **Service** | **UID** | **Characteristic** | **UID** | **Mode** | **Comments** |
| Pulse stream | AC7744C0-0BAD-11EF-A9CD-0800200C9A00 |  |  |  |  |
|  |  | Pulse stream | AC7744C0-0BAD-11EF-A9CD-0800200C9A01 | Write cmd | See "Sending individual pulses" / "Packet format" section |
|  |  | Triphase permitted | AC7744C0-0BAD-11EF-A9CD-0800200C9A02 | Read | 0x01 if disabling channel isolation (required for triphase) is permitted, 0x00 otherwise |
|  |  | Channel isolation | AC7744C0-0BAD-11EF-A9CD-0800200C9A03 | Write req | Write 0x00 to disable channel isolation, 0x01 to enable (default) |
|  |  |  |  |  |  |
| General control service | AC7744C0-0BAD-11EF-A9CD-0800200C9B00 |  |  |  |  |
|  |  | Channel 1 dial | AC7744C0-0BAD-11EF-A9CD-0800200C9B01 | Read, notify | Power set on front panel. 0-1000 |
|  |  | Channel 2 dial | AC7744C0-0BAD-11EF-A9CD-0800200C9B02 | Read, notify | Power set on front panel. 0-1000 |
|  |  | Channel 3 dial | AC7744C0-0BAD-11EF-A9CD-0800200C9B03 | Read, notify | Power set on front panel. 0-1000 |
|  |  | Channel 4 dial | AC7744C0-0BAD-11EF-A9CD-0800200C9B04 | Read, notify | Power set on front panel. 0-1000 |
|  |  |  |  |  |  |
|  |  | Channel 1 pulse width | AC7744C0-0BAD-11EF-A9CD-0800200C9B11 | Write cmd | Two bytes, each 0-0xFF, for pulse widths of positive and negative pulses of 0-255us |
|  |  | Channel 2 pulse width | AC7744C0-0BAD-11EF-A9CD-0800200C9B12 | Write cmd | Two bytes, each 0-0xFF, for pulse widths of positive and negative pulses of 0-255us |
|  |  | Channel 3 pulse width | AC7744C0-0BAD-11EF-A9CD-0800200C9B13 | Write cmd | Two bytes, each 0-0xFF, for pulse widths of positive and negative pulses of 0-255us |
|  |  | Channel 4 pulse width | AC7744C0-0BAD-11EF-A9CD-0800200C9B14 | Write cmd | Two bytes, each 0-0xFF, for pulse widths of positive and negative pulses of 0-255us |
|  |  |  |  |  |  |
|  |  | Channel 1 frequency | AC7744C0-0BAD-11EF-A9CD-0800200C9B21 | Write cmd | 0x01 to 0xFF for 1-255 hz |
|  |  | Channel 2 frequency | AC7744C0-0BAD-11EF-A9CD-0800200C9B22 | Write cmd | 0x01 to 0xFF for 1-255 hz |
|  |  | Channel 3 frequency | AC7744C0-0BAD-11EF-A9CD-0800200C9B23 | Write cmd | 0x01 to 0xFF for 1-255 hz |
|  |  | Channel 4 frequency | AC7744C0-0BAD-11EF-A9CD-0800200C9B24 | Write cmd | 0x01 to 0xFF for 1-255 hz |
|  |  |  |  |  |  |
|  |  | Channel 1 power level | AC7744C0-0BAD-11EF-A9CD-0800200C9B31 | Write cmd | Power level, 0-1000 for 0-100%, but limited to value set by front panel dial |
|  |  | Channel 2 power level | AC7744C0-0BAD-11EF-A9CD-0800200C9B32 | Write cmd | Power level, 0-1000 for 0-100%, but limited to value set by front panel dial |
|  |  | Channel 3 power level | AC7744C0-0BAD-11EF-A9CD-0800200C9B33 | Write cmd | Power level, 0-1000 for 0-100%, but limited to value set by front panel dial |
|  |  | Channel 4 power level | AC7744C0-0BAD-11EF-A9CD-0800200C9B34 | Write cmd | Power level, 0-1000 for 0-100%, but limited to value set by front panel dial |
|  |  |  |  |  |  |
|  |  | Channel 1 power enable | AC7744C0-0BAD-11EF-A9CD-0800200C9B41 | Write cmd | 0 = power off, 1 = power on |
|  |  | Channel 2 power enable | AC7744C0-0BAD-11EF-A9CD-0800200C9B42 | Write cmd | 0 = power off, 1 = power on |
|  |  | Channel 3 power enable | AC7744C0-0BAD-11EF-A9CD-0800200C9B43 | Write cmd | 0 = power off, 1 = power on |
|  |  | Channel 4 power enable | AC7744C0-0BAD-11EF-A9CD-0800200C9B44 | Write cmd | 0 = power off, 1 = power on |
|  |  |  |  |  |  |
| Device Information | 0x180A |  |  |  | ORG_BLUETOOTH_SERVICE_DEVICE_INFORMATION |
|  |  | Manufacturer | 0x2A29 | Read |  |
|  |  | Model number | 0x2A24 | Read | “ZC95” |
|  |  | Firmware version | 0x2A26 | Read | E.g. “v1.9”, matches ZC95 f/w version displayed in about screen |
|  |  |  |  |  |  |
| Battery service | 0x180F |  |  |  | ORG_BLUETOOTH_SERVICE_BATTERY_SERVICE |
|  |  | Battery level | 0x2A19 | Read, notify | 0-100 |

[p1]: images/diagrams/pulse_pair.png "Pulse pair"
[p2]: images/diagrams/single_pulse.png "Single pulse"
[p150hz]: images/diagrams/150hz.png "150hz signal"
