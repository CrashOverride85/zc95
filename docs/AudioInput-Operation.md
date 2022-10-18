# Audio Input

If the (optional) audio input board is present, an extra "Audio input" option shows on the settings menu, along with patterns using audio input.

Audio input can be either line level or mic level (selectable), and optionally power can be provided for mics. It's intended to be used with PC style electret microphones. I've found this microphone works particularly well (and with the mk312bt): https://www.amazon.co.uk/gp/product/B07L6H7YYQ/

When the audio input board is installed, the Aux port on the front can be configured for either Audio input (default) or serial I/O (currently only debug out).

## Configuration
There are two relevant menus for configuring audio - "Audio input" and "Hardware config" ("Audio" and "Aux port use").

### Audio input
This is used to set gain, enable/disable microphone preamp (so set for mic or line level), enable/disable microphone power, and show a preview of the audio signal to aid setting volume and gain.
The configuration set here will be used in patterns, and will be retained after power off. 

On opening the screen, it should show the currently configured settings. E.g.:

![Audio input mic]

The above screenshot is when in microphone mode, with the microphone power disabled (using a self powered electret mic). Options:
* Ddbl mic pre: Disable microphone preamp - i.e. switch to line level input
* Enbl mic pwr: Enable microphone power - provide power for an unpowered electret microphone. Note this should not be used with dynamic mics or when connected to a line level output

Disabling the mic preamp switches to line level input, and shows stereo input:

![Audio input line]


### Hardware config
There are two relevant entries in the hardware config menu:

1. Audio. 
This is used to control audio functionality, and has 3 options
    - Auto - Default. If the audio output board is detected (specifically, if the MCP4651 digital potentiometer is found), then audio functionality is enabled, otherwise it's hidden.
    - On (no gain) - Audio input functionality is enabled regardless of the audio board being detected or not. But the gain/volume option is removed, as that requires the digital potentiometer to be present.
    - Off - hide audio functionality even if audio input board found

2. Aux port use. 
With the audio board present, the zc95 has the ability to switch the use of the port between serial I/O and audio input. At present, the only use for the serial option is outputting debugging info, but in the future some patterns may make use of it. Two options:
    - Audio input - Default. Port is used for audio input
    - Serial I/O - port is used for serial I/O

Note this option has no effect if the audio input board isn't present; the port is always routed to the serial interface.



[Audio input mic]: images/screen_audio_input_mic.jpg "Audio input configuration screen - mic"
[Audio input line]: images/screen_audio_input_line.jpg "Audio input configuration screen - line input"
