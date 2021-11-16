#ifndef _CSAVEDSETTINGS_H
#define _CSAVEDSETTINGS_H

#include "CEeprom.h"
#include "CChannel_types.h"

#define EEPROM_SIZE      512 // EEPROM is 4Kbit
#define EEPROM_MAGIC_VAL  85 // If this is in setting::EepromInit, assume the eeprom has been initialised

#define EEPROM_CHANNEL_COUNT 10 // Maximum number of configuerd channels that can be saved. Changing this invalidates EEPROM contents

class CSavedSettings
{

    enum class setting
    {
        EepromInit     =  0, // Contains magic value if eeprom as been initialised
        ChannelType    =  1, // Channel id 0 type
        ChannelIndex   =  2, // Channel id 0 index
        //  ...        = 19     Channel id 9 type
        //  ...        = 20     Channel id 9 index
        LEDBrightness  = 21, // default LED brightness
        PowerStep      = 22, // When using rot encoder for power, how many steps to increase power by for each click (e.g. as power level is 0-1000, 10 would give 100 possible power levels)
        RampUpTimeSecs = 23, // When selecting a routine, how long (in seconds) does the power take to reach the power level set on the front pannel
        // To Fill

        // Collar config
        Collar0IdLow   = 30,
        Collar0IdHigh  = 31,
        Collar0Chan    = 32,
        Collar0Mode    = 33,
       // not used     = 34
       // Collar1IdLow = 35
       // ...
       // Collar9Mode  = 78
       // not used     = 79
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

        // Collar
        bool get_collar_config(uint8_t collar_id, struct collar_config &collar_conf);
        bool set_collar_config(uint8_t collar_id, struct collar_config &collar_conf);


        void eeprom_initialise();
    private:
        bool eeprom_initialised();
        
        void initialise_collar(uint8_t collar_id);
        CEeprom *_eeprom;
        uint8_t _eeprom_contents[EEPROM_SIZE];
};

#endif
