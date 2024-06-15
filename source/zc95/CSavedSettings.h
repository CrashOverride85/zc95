#ifndef _CSAVEDSETTINGS_H
#define _CSAVEDSETTINGS_H

#include "bluetooth.h"
#include "Bluetooth/CBluetoothRemote.h"

#include "CEeprom.h"
#include "CChannel_types.h"

#define EEPROM_SIZE      512 // EEPROM is 4Kbit
#define EEPROM_MAGIC_VAL  85 // If this is in setting::EepromInit, assume the eeprom has been initialised

#define EEPROM_CHANNEL_COUNT 10 // Maximum number of configured channels that can be saved. Changing this invalidates EEPROM contents

class CSavedSettings
{
    enum class setting
    {
        EepromInit       =  0, // Contains magic value if eeprom as been initialised
        ChannelType      =  1, // Channel id 0 type
        ChannelIndex     =  2, // Channel id 0 index
        //  ...          = 19     Channel id 9 type
        //  ...          = 20     Channel id 9 index
        LEDBrightness    = 21, // default LED brightness
        PowerStep        = 22, // When using rot encoder for power, how many steps to increase power by for each click (e.g. as power level is 0-1000, 10 would give 100 possible power levels)
        RampUpTimeSecs   = 23, // When selecting a routine, how long (in seconds) does the power take to reach the power level set on the front panel
        AudioGainL       = 24, // Gain for left  channel
        AudioGainR       = 25, // Gain for right channel
        MicPreAmp        = 26, // Mic pre-amp enabled
        MicPower         = 27, // Mic power enabled
        Audio            = 28, // Audio setting: Auto/no gain/off
        Debug            = 29, // Debugging output destination
    
        // Collar config
        Collar0IdLow     = 30,
        Collar0IdHigh    = 31,
        Collar0Chan      = 32,
        Collar0Mode      = 33,
       // not used       = 34
       // Collar1IdLow   = 35
       // ...
       // Collar9Mode    = 78
       // not used       = 79

        AuxPort          = 80,  // Aux port usage - Audio or Serial
        WifiSSID         = 81,  // SSID to connect to. 32 characters/bytes + null
        WifiPSK          = 114, // Password, 64 characters + null
        WiFiConfigured   = 179, // Set to EEPROM_MAGIC_VAL if wifi configured. Anything else means not configured.
        WifiApPsk        = 180, // Auto-generated AP PSK. 16 characters + NULL
        WifiApPskEnd     = 196, //
        PowerLevelDisp   = 197, // Power level numeric display 
        ButtonLedBright  = 198, // Brightness of illuminated LED buttons 
        BluetoothOn      = 199, // Bluetooth enabled yes/no
        BTAddrStart      = 200, // Bluetooth address (6 bytes) start. Currently selected / paired device
        BTAddrEnd        = 205, // Bluetooth address end

        // These map keypress_t::* to a keypress_action_t::*, e.g. KEY_LEFT => ROT_LEFT
        BTButtonAction   = 206, // KEY_BUTTON
        BTUpAction       = 207, // KEY_UP
        BTDownAction     = 208, // KEY_DOWN
        BTLeftAction     = 209, // KEY_LEFT
        BTRightAction    = 210, // KEY_RIGHT
        BTShutterAction  = 211, // KEY_SHUTTER
        BTUnknownAction  = 212, // KEY_UNKNOWN
     //<next bt action>= 213,
        BtDeviceType     = 220, // Bluetooth device type (HID, GENERIC, etc.)
        BleAllowTriphase = 221, // If enabled, channel isolation can be disabled via BLE remote connect
        BlePowerMode     = 222, // What the front panel power dials do. See ble_power_dial_mode_t.
        PowerLevel       = 223  // Power level: Low, Medium or High (default)
    };

    public:
        struct channel_selection
        {
            CChannel_types::channel_type type;
            uint8_t index;
        };

        struct collar_config
        {
            uint16_t id;
            uint8_t channel;
            uint8_t mode;
        };

        // Possible options for various saved settings 
        enum class setting_audio
        {
            AUTO     = 0,   // Enable audio logic if digipot found, disable otherwise
            NO_GAIN  = 1,   // Enable audio logic even if digipot not found, but disable setting of gain which requires digipot
            OFF      = 2    // Disable audio logic even if digipot is found
        };

        enum class setting_debug
        {
            ACC_PORT = 0,  // Output debugging info on accessory port
            AUX_PORT = 1,  // Output debugging info on aux port
            OFF      = 2   // Disable debugging output
        };

        enum class setting_aux_port_use
        {
            AUDIO  = 0, // Aux port is routed to ADC if audio board present (same as SERIAL if not present)
            SERIAL = 1  // Aux port is routed to AUX_PORT_UART (uart0)
        };

        enum class power_level_show_percent
        {
            OFF                = 0,   // Do not show numeric power level anywhere
            DISAPPEARING_TEXT  = 1,   // Show power level on change for a few moments
            IN_BAR_GRAPH       = 2,   // Always show power level as a number in the bar graph
            BOTH               = 3    // DISAPPEARING_TEXT & IN_BAR_GRAPH
        };

        enum class bt_device_type_t
        {
            HID          = 0, // hopefully a bluetooth shutter remote
            NOT_RECEIVED = 1, // not in advertising report
            OTHER        = 2, // in advertising report, but not handled
        };

        // When in BLE remote access mode, what the front panel power dials do
        enum class ble_power_dial_mode_t
        {
            LIMIT = 0,
            SCALE = 1
        };

        enum class power_level_t
        {
            HIGH   = 0,
            MEDIUM = 1,
            LOW    = 2
        };

        CSavedSettings(CEeprom *eeprom);
        ~CSavedSettings();
        void save();

        // Channel
        channel_selection get_channel(uint8_t channel_id);
        void set_channel(channel_selection chan, uint8_t channel_id);

        // LED brightness        
        uint8_t get_led_brightness();
        void set_led_brightness(uint8_t led_brightness_percent);

        // Power level step interval
        uint8_t get_power_step_interval();
        void set_power_step_interval(uint8_t power_step);

        // Power ramp up time, seconds
        uint8_t get_ramp_up_time_seconds();
        void set_ramp_up_time_seconds(uint8_t time_secs);

        // Gain for audio input
        uint8_t get_audio_gain_left();
        uint8_t get_audio_gain_right();
        void set_audio_gain_left(uint8_t gain);
        void set_audio_gain_right(uint8_t gain);

        // Mic pre amp. Needs to be turned on to use a microphone on the audio input (line level with it off)
        bool get_mic_preamp_enabled();
        void set_mic_preamp_enabled(bool enabled);

        // Enable mic power. Needs to be on (along with the mic preamp) to use electret microphones
        bool get_mic_power_enabled();
        void set_mic_power_enabled(bool enabled);

        setting_audio get_audio_setting();
        void set_audio_setting(setting_audio setting);

        setting_debug get_debug_dest();
        void set_debug_dest(setting_debug setting);

        // Aux port use
        setting_aux_port_use get_aux_port_use();
        void set_aux_port_use(setting_aux_port_use setting);

        // Wifi credentials. Returns true if credentials returned (wifi already configured)        
        bool get_wifi_credentials(std::string &out_ssid, std::string &out_password);
        bool set_wifi_credentials(std::string ssid, std::string password); // Returns true if saved, false otherwise (e.g. too long)
        void clear_wifi_credentials();
        bool wifi_is_configured();

        // Wifi AP mode psk. Autogenerated on first run. The idea behind storing it is that it's
        // probably annoying if it changes every time you start AP mode.
        bool get_wifi_ap_psk(std::string &out_psk);
        bool set_wifi_ap_psk(std::string psk);
        void clear_saved_ap_psk();

        // Numeric power level display
        power_level_show_percent get_power_level_display();
        bool power_level_show_in_bar_graph();
        bool power_level_show_disappearing_text();
        void set_power_level_display(power_level_show_percent setting);

        // Illuminated button brightness        
        uint8_t get_button_brightness();
        void set_button_brightness(uint8_t button_brightness_byte);

        // Collar
        bool get_collar_config(uint8_t collar_id, struct collar_config &collar_conf);
        bool set_collar_config(uint8_t collar_id, struct collar_config &collar_conf);

        // Bluetooth enabled
        bool get_bluethooth_enabled();
        void set_bluethooth_enabled(bool setting);

        // Address of paired bluetooth device
        void get_paired_bt_address(bd_addr_t *address);
        void set_paired_bt_address(bd_addr_t address);

        // Bluetooth remote button to action mappings
        CBluetoothRemote::keypress_action_t get_bt_keypress_action(CBluetoothRemote::keypress_t key);
        void set_bt_keypress_action(CBluetoothRemote::keypress_t key, CBluetoothRemote::keypress_action_t action);

        // Paired bluetooth device type
        bt_device_type_t get_paired_bt_type();
        void set_paired_bt_type(bt_device_type_t type);

        // Allow BLE remote connections to disable channel isolation
        bool get_ble_remote_disable_channel_isolation_permitted();
        void set_ble_remote_disable_channel_isolation_permitted(bool setting);

        // Front panel power dial mode when running BLE remote access
        ble_power_dial_mode_t get_ble_remote_access_power_dial_mode();
        void set_ble_remote_access_power_dial_mode(ble_power_dial_mode_t mode);

        // Power level
        power_level_t get_power_level();
        void set_power_level(power_level_t power_level);

        void eeprom_initialise();

    private:
        bool eeprom_initialised();
        
        void initialise_collar(uint8_t collar_id);
        CEeprom *_eeprom;
        uint8_t _eeprom_contents[EEPROM_SIZE];
};

#endif
