# JSON interface

The ZC95 can be controlled remotely using a JSON based interface over either serial or a WiFi/Websocket connection.

Consider this a work in progress, with the messages subject to change for the time being.
These messages primarily exist to allow the supplied python utilities (see [Remote Access](./RemoteAccess.md)) to interact with the box, and have been available since firmware version 1.4.

These allow for Lua scripts to be uploaded, deleted, started, and controlled. Unlike a [BLE connection](./BleStreaming.md), it does not allow direct control of the estim output / per pulse control. It can be used to manage the scripts on the box, and start + control a script.

The messages sent are the same regardless of connection method.

## Connection method
### Websocket
After configuring WiFi and connecting, connect to the box on `ws://<ip>:80/stream`. There is a fairly short timeout / idle disconnection period, so I would recommend setting a low ping interval on the connection - e.g. 6 seconds.

On disconnection, any running pattern will be stopped.

### Serial
Connect using 115200-8-N-1. All messages should be surrounded with STX (0x02) / ETX (0x03) characters, e.g. 

```<STX>{"Type": "GetLuaScripts", "MsgId": 3}<ETX>```

Messages sent from the ZC95 are also sent surrounded by STX/ETX. Any data received from the ZC95 not enclosed with STX/ETX should be ignored. 

An EOT (0x04) character can be sent at any point to reset the connection - this has much the same effect as disconnecting if using a web socket connection, and will stop any running pattern and reset power levels.

Consider sending an EOT, then waiting ~50ms, on first connection to make sure the ZC95 is in a known state. The python utilities currently don't do this. They probably should.

## States
The connection can be in one of three states, with the initial state being `ACTIVE`. The state affects which messages are valid.

![states diag]

The LUA_LOAD is used to upload a script, ROUTINE_RUN to run and control a pattern, and ACTIVE for everything else.

## Messages
### Structure
All messages sent should have at least these two fields:
* `Type` - type / name of message 
* `MsgId` - incrementing message counter. This will be present in the response from the ZC95

The ZC95 will respond to all messages sent to it, and at a minimum the response will include:
* `Type` - will either be specific to the request (e.g. if `GetLuaScripts` was sent, the response would have a type of `LuaScripts`), or just `Ack`
* `MsgId` - id of the message that it is a response to
* `Result` - Either `OK` on success, or `ERROR` on failure

On error, the response _may_ also include an `Error` field, which includes an error message with more details about the problem.

Some messages (e.g. power level change) will be sent by the ZC95 not linked to any request - these will have the `MsgId` set to `-1`, and will not include a `Result` field.

### Messages for state: ACTIVE
In the active state, these messages are recognised:

* GetLuaScripts
* DeleteLuaScript
* GetPatterns
* GetPatternDetail
* GetVersion
* LuaStart
* PatternStart

#### GetLuaScripts
Returns a list of all uploaded Lua scripts, e.g.:

Send:
```
{"Type": "GetLuaScripts", "MsgId": 3}
```

Response:
```
{
   "Type":"LuaScripts",
   "MsgId":3,
   "Scripts":[
      {
         "Index":4,
         "Empty":true,
         "Valid":false,
         "Name":"<empty>"
      },
      {
         "Index":3,
         "Empty":true,
         "Valid":false,
         "Name":"<empty>"
      },
      {
         "Index":2,
         "Empty":true,
         "Valid":false,
         "Name":"<empty>"
      },
      {
         "Index":1,
         "Empty":false,
         "Valid":true,
         "Name":"U:Test Script"
      },
      {
         "Index":0,
         "Empty":true,
         "Valid":false,
         "Name":"<empty>"
      }
   ],
   "Result":"OK"
}
```
Assume scripts array is unordered, as the order may change in future versions.

Fields:
- `Index` - script slot. Currently 0-4, may be extended in the future
- `Empty` - true if there is no script in the slot
- `Valid` - true if there is a script uploaded, and it parses ok
- `Name` - name of script as displayed on the zc95 scripts menu

#### DeleteLuaScript
Clears a Lua script slot, removing the script from the menu. It is _not_ necessary to call this before uploading a new script to an in-use slot.

There is no error when deleting an already empty script slot.

Send:
```
{"Type": "DeleteLuaScript", "MsgId": 5, "Index": 1}
```

Response:
```
{"Type":"Ack","MsgId":5,"Result":"OK"}
```

#### GetPatterns
Returns a list of all patterns that can be ran remotely - i.e. including uploaded scripts along with built in patterns.

Send:
```
{"Type": "GetPatterns", "MsgId": 4}
```

Response:
```
{
   "Type":"PatternList",
   "MsgId":4,
   "Patterns":[
      {
         "Id":0,
         "Name":"U:Test script"
      },
      {
         "Id":1,
         "Name":"Waves"
      },
      {
         "Id":2,
         "Name":"Orgasm"
      },
      {
         "Id":3,
         "Name":"Climb"
      }

      ....
   ],
   "Result":"OK"
}
```

#### GetPatternDetail
Get all details required to show an option screen for a specific pattern. Expects an `Id` parameter that corresponds to pattern id as returned in the `PatternList` message (i.e. not a slot number/index from the `LuaScripts` message).

The contents of the `PatternDetail` message is mostly generated from, and so closely related to, the Config block in Lua scripts.

```
{"Type": "GetPatternDetail", "MsgId": 6, "Id": 11}
```

Response:
```
{
   "Type":"PatternDetail",
   "MsgId":6,
   "Name":"Toggle",
   "Id":11,
   "ButtonA":"",
   "MenuItems":[
      {
         "Id":1,
         "Title":"Speed",
         "Group":0,
         "Type":"MIN_MAX",
         "Min":500,
         "Max":4000,
         "IncrementStep":50,
         "UoM":"mHz",
         "Default":2000
      },
      {
         "Id":2,
         "Title":"Pulse/Cont.",
         "Group":0,
         "Type":"MULTI_CHOICE",
         "Default":0,
         "Choices":[
            {
               "Id":0,
               "Name":"Continuous"
            },
            {
               "Id":1,
               "Name":"Pulse"
            }
         ]
      }
   ],
   "Result":"OK"
}
```

#### GetVersion
Used to query the zc95 for its version.

Send:
```
{"Type": "GetVersion", "MsgId": 2}
```

Response:
```
{"Type":"VersionDetails","MsgId":2,"ZC95":"v2.0-rc3-7-g052b00a","WsMajor":1,"WsMinor":0,"SerialNo":"E661640843703B2B","Result":"OK"}
```

The `ZC95` field holds the firmware version of the zc95 - this will match the text displayed in grey at the bottom of the ZC95 boot screen.

`WsMajor` is likely to be increased for breaking changes to this JSON interface, and the `WsMinor` for additional features. For time being, don't trust them, and consider everything on this page to be work in progress. At some point I'll start incrementing them, and keeping a proper change log for the messages.

#### LuaStart
Used to start the upload of a Lua script, and switch to the `LUA_LOAD` state on success. Expects an `Index` parameter, which is the script slot (0-4) to upload the following script to.

Send:
```
{"Type": "LuaStart", "Index": 0, "MsgId": 2}
```

Response:
```
{"Type":"Ack","MsgId":2,"Result":"OK"}
```

#### PatternStart
Used to start a pattern, and on success, switches to the `ROUTINE_RUN` state. Expects one parameter, `Index`, which is the script id (as returned in the `PatternList` message) to start.

Send:
```
{"Type": "PatternStart", "MsgId": 7, "Index": 1}
```

Response:
```
{"Type":"Ack","MsgId":7,"Result":"OK"}
```


### Messages for state: LUA_LOAD
Entered following a successful `LuaStart` message.
Used to upload a new Lua script. 

The expected sequence is:
- LuaStart
- 1 LuaLine message per line in the script
- LuaEnd

In this state, these messages are recognised:

* LuaLine
* LuaEnd

#### LuaLine
Send a line of a Lua script

Send:
```
{"Type": "LuaLine", "LineNumber": 0, "Text": "_delay_ms = 500", "MsgId": 3}
```

Response:
```
{"Type":"Ack","MsgId":3,"Result":"OK"}
```

Wait for the `Ack` for each line before sending the next `LuaLine` message.
`LineNumber` should be incremented for each line sent.

#### LuaEnd
Send after the final `LuaLine` message has been sent and Ack'd. Causes the zc95 to parse the script, and make it available in the Menu if it appears to be valid.

Even on failure, it is likely that the previous script in the target slot/index will have been removed.

Success or failure, the state will return to `ACTIVE` after this message.

Send:
```
{"Type": "LuaEnd", "MsgId": 90}
```

Response:
```
{"Type":"Ack","MsgId":90,"Result":"OK"}
```

### Messages for state: ROUTINE_RUN
This state is entered following a successful `PatternStart` message.

In this state, these messages are recognised:

* PatternMinMaxChange
* PatternMultiChoiceChange
* PatternSoftButton
* PatternStop
* SetPower

Additionally, the zc95 can send these messages at any time whilst in this state (with a `MsgId` of -1):

* LuaScriptError
* LuaScriptOutput
* MenuOptionChanged
* PowerStatus

#### PatternMinMaxChange
Send to change the value of a min/max pattern parameter.

Send:
```
{"Type": "PatternMinMaxChange", "MsgId": 5, "MenuId": 1, "NewValue": 5100}
```

Response:
```
{"Type":"Ack","MsgId":5,"Result":"OK"}
```

#### PatternMultiChoiceChange
Send to change the value of a multi choice pattern parameter.

Send:
```
{"Type": "PatternMultiChoiceChange", "MsgId": 5, "MenuId": 2, "ChoiceId": 1}
```

Response:
```
{"Type":"Ack","MsgId":5,"Result":"OK"}
```

#### PatternSoftButton
Send to simulate the top left soft button being pressed or released. Send with Pressed=1 to "press" the button, then Pressed=0 to release.

To avoid unexpected results, always send a Pressed=1 message followed by a Pressed=0 message, with similar timings to the button being physically pushed/released.

Send:
```
{"Type": "PatternSoftButton", "MsgId": 5, "Pressed": 1}
```

Response:
```
{"Type":"Ack","MsgId":5,"Result":"OK"}
```

#### PatternStop
Stops the running pattern, and returns to the `ACTIVE` state.

Either disconnecting if using the web socket interface, or sending an EOT (0x04) over serial, will achieve a similar result.

Send:
```
{"Type": "PatternStop", "MsgId": 4}
```

Response:
```
{"Type":"Ack","MsgId":4,"Result":"OK"}
```

#### SetPower
Set the power level for each channel, 0-1000. The front panel dials act as an upper limit to power level. E.g if the front panel is set to 50%, and a power level of 1000 is sent, the actual power level will be set to 500.

Send:
```
{"Type": "SetPower", "MsgId": 5, "Chan1": 0, "Chan2": 0, "Chan3": 0, "Chan4": 0}
```

Receive:
```
{"Type":"Ack","MsgId":5,"Result":"OK"}
```

#### LuaScriptError
Sent if a Lua script errors out and is stopped. Equivalent to the display showing a red x and "script error"

A `LuaScriptOutput` message will also usually be sent if more details are available. 

Receive:
```
{"Type":"LuaScriptError","MsgId":-1}
```

#### LuaScriptOutput
Sent either if `print("<message>")` is called from script, or if a script fails/errors out.

Fields:
* `TextType` will either be `Print` for messages from a `print` statement, or `Error` for error messages
* `Text` is either the message from a `print` statement, or an error message
* `Time` corresponds to the time the message was generated/queued to send. It is the number of microseconds since the box was powered on.

Receive:
```
{"Type":"LuaScriptOutput","MsgId":-1,"Text":"Script stopped... error: \n[string \"...\"]:34.0000: attempt to call field 'fff' (a nil value)","Time":3856714853,"TextType":"Error"}
```

#### MenuOptionChanged
Sent if a script changes one of its own parameters by calling `zc.SetMenuOption`. At present, none of the inbuilt pattern do this, but the option is there for uploaded Lua scripts.

Fields:
* `MenuId` - corresponds to Config.menu_items.id in the Lua script
* `Value` - for MIN_MAX type menu entries, will be between the min and max value. For MULTI_CHOICE menu entries, it will be one of the choice_id's for that entry

Receive:
```
{"Type":"MenuOptionChanged","MsgId":-1,"MenuId":1,"Value":500}
```

#### PowerStatus
Sent whenever the front panel dials are changed, or the script changes the requested output power.

Fields:
* `OutputPower` - 0-1000. Actual output power after front panel power limit, power sent in the `SetPower` message, and level requested by script considered. Corresponds to the yellow bar on the display
* `MaxOutputPower` - 0-1000. The lower of the power set for the chanel using the `SetPower` message, and the limit set on the front panel
* `PowerLimit` - 0-1000. Power limit set on the front panel

Receive:
```
{
   "Type":"PowerStatus",
   "MsgId":-1,
   "Channels":[
      {
         "Channel":1,
         "OutputPower":0,
         "MaxOutputPower":0,
         "PowerLimit":183
      },
      {
         "Channel":2,
         "OutputPower":0,
         "MaxOutputPower":0,
         "PowerLimit":0
      },
      {
         "Channel":3,
         "OutputPower":0,
         "MaxOutputPower":0,
         "PowerLimit":1000
      },
      {
         "Channel":4,
         "OutputPower":1000,
         "MaxOutputPower":1000,
         "PowerLimit":1000
      }
   ]
}
```

[states diag]: images/states.png "Valid states"
