# Remote access

***Warning***: All remote access and Lua stuff is somewhat experimental at this point

## Summary
If a Pico-W is used for the MCU on the main board, a "Remote access" menu is available in the config menu.
The remote access options can be used to:
* Upload Lua scripts (of which 5 can be stored)
* Control the box remotely with a Python GUI

At present, other than the initial WiFi setup, there is no web interface (yet).

## Configuring Wifi
The basic process is use the "Config Wifi/AP mode" option, ideally connect to the ZC95 using a phone, then use the web interface to enter a WiFi SSID/Password. When the "Connect to Wifi" option is next used, these credentials will be used. At present, other than setting the WiFi SSID/password, nothing else can be done in ap mode.

When selecting "Config Wifi/AP mode", a screen showing the strongest networks found is displayed:

![Scanning]

This will continue to scan for networks for as long as the screen is open.

When at least one WiFi network is detected, the "Start AP" soft button (top right) is enabled. Pressing this starts the Access Point mode, and shows a QR code to allow connecting:

![QrCode]

### Apple phones
Scan the QR code - you should get a popup to connect to the zc95-xxx WiFi network - press join. Then go to settings -> Wi-Fi, you should get a popup titled "Log in" followed by this screen (can take 4-5 seconds or so to appear):

![PhoneConfigWifi]

Go into "Change wifi settings", it should list all WiFi networks found. Pick the appropriate one, then enter the password for it when prompted then "Connect to the wifi network". 
That should be it - exit the menu on the ZC95 and try connecting to the network.

If the QR code isn't working for some reason, press the top left ("SSID/PASS") soft button and you should see a screen giving the SSID and password required to connect manually, e.g.:

![ShowSsidAndPassword]


If the config page doesn't automatically appear, manually browse to "http://192.168.4.1/" once connected to the hotspot.

### Android
Untested, but hopefully similar.

## Connecting to Wifi
Select the "Connect to WiFi" option, and it should connect to the WiFi network previously configured using the above steps. Once connected, you should see something that looks like:

![ConnectedToWifi]

At this point, the Python GUI and scripts to list/upload Lua scripts should work if given the IP displayed on screen.


## Python scripts
There are a few Python scripts available in the `remote_access` folder for interacting with the ZC95 once it's connected to wifi:
* pattern_list.py - lists all patterns available remotely, along with the ID number (excludes Audio patterns)
* lua_manage.py - lists uploaded Lua scripts, and allows for them to be deleted
* lua_upload.py - uploads a Lua script
* pattern_gui.py - starts a GUI that starts a pattern and can then be used to control it

(all scripts can be given an optional `--debug` parameter which shows messages being sent/received, and sometimes other extra info)

### pattern_list.py
Used to list all pattens on the ZC95 that can be controlled remotely, including any from Lua scripts uploaded. E.g.:
```
$ python3 pattern_list.py --ip 192.168.1.137
Connecting
Connection opened
Patterns on ZC95:
0       U:Waves
1       Waves
2       Toggle
3       RoundRobin
4       TENS
5       Climb
6       Triggered Climb
7       Fire
12      Climb with pulse
13      Predicament
14      Shock choice
15      Camera Trigger
16      Buzz
Websocket connection closed
$
```
All uploaded Lua scripts are prefixed with "U:". Note that the pattern ID listed isn't fixed - it will change as extra Lua scripts are uploaded and if the pattern order is changed in future versions.

Finally, the Audio patterns are excluded for the time being as these do not work remotely.

### lua_manage.py
Can be used to list and delete Lua scripts on the box, e.g.:
```
$ python3 lua_manage.py --ip 192.168.1.137 --list
Connecting
Connection opened
Script slots on ZC95:
5 - <empty>
4 - <empty>
3 - <empty>
2 - <empty>
1 - U:Waves
Websocket connection closed
$
```
When uploading a Lua script, it's the id/index from the above list that needs to be used to specify where to put the script.

A script can be deleted by running `lua_manage.py --ip 192.168.1.137 --delete <index>`, but it can be overwritten without being deleted.

### lua_upload.py
Uploads a Lua script to the designated slot/index on the ZC95, e.g.:
```
$ python3 lua_upload.py --script ./lua/waves.lua --ip 192.168.1.137 --index 1
Connecting
Connection opened
Uploading...
Done!
Websocket connection closed
$ 
```

If the script isn't valid, you may see an error like:
```
$ python3 lua_upload.py --script ./lua/waves.lua --ip 192.168.1.137 --index 1
Connecting
Connection opened
Uploading...
Got error message: 
    Invalid script: [string "_freq_min_hz = 25..."]:9.00000: '=' expected near 'Config'
Websocket connection closed
Failed
$ 
```
The "9.00000" means the error is likely on/around line `9`; I'm not sure why Lua includes the `.00000`. 

Note that many errors won't be picked up at this stage - a successful upload doesn't mean the script is good.

Once uploaded, the script will be visible at the top of the usual patterns list, as well as being available for pattern_gui.py to control.

See [Lua](./LuaNotes.md) for notes on writing Lua scripts.

### pattern_gui.py
See next section for more details on running a pattern remotely. 


## Running a pattern remotely

### Power setting
When running a pattern remotely, the 4 power dials on the front panel are used to set the maximum power setting possible for the respective channel. I.e. setting all channels to maximum means the box can be fully controlled, up to maximum power, remotely. 

### Starting
Start by passing the index (as given by `pattern_list.py`) of the pattern to run to the pattern_gui.py script:
```
$ python3 pattern_gui.py --ip 192.168.1.137 --index 1
Connecting
Connection opened
```

After connecting the GUI should then appear:

![Gui]

The 4 channels are set to different levels:
1. Front panel is set to full power, and the GUI is slider is about half way; power is as set on the GUI
2. Front panel is set to just over half power, but the GUI slider is set to full power; power is limited to just over half
3. Front panel is set low, but GUI is set to zero; power is 0%
4. Front panel and GUI is at minimum; power is 0%

### Debug mode
If started with the ```--debug``` flag, in addition to showing messages sent/received, an output window appears at the bottom of the GUI:

![GuiDebug]

In this case, it's showing a script that's failed on line 112 due to a call to a function that doesn't exist. Any `print()` output from Lua scripts will also appear here, making this mode useful for testing new Lua scripts.


[Scanning]: images/screen_ra_scanning.jpg "Remote access (AP mode) scanning screen"
[QrCode]: images/screen_ra_qr.jpg "Remote access (AP mode) screen showing QR code"
[PhoneConfigWifi]: images/phone_config_wifi.jpg "Configure wifi on iphone"
[ShowSsidAndPassword]: images/screen_ra_ap.jpg "Screen showing SSID and password for AP mode"
[ConnectedToWifi]: images/screen_ra_connected.jpg "Connected to WiFi screen"
[Gui]: images/pattern_gui_start.png "Python GUI showing Waves pattern"
[GuiDebug]: images/gui_debug_window.png "Python GUI showing debug window"
