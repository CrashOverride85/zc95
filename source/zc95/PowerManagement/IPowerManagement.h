#ifndef _IPOWERMANAGEMENT_H_
#define _IPOWERMANAGEMENT_H_

#include <string>
#include <inttypes.h>

/**
 * @brief Power management functions to get power status - charging/charged/plugged in/on battery/battery voltage
 * 
 */
class IPowerManagement
{
    public:
        enum class charging_status_t
        {
            Charging,
            Charged,
            Unknown,  // Either on battery (discharging), or error determining state (fault, missing bat, etc.)
            NA        // No hardware support to report charge status (i.e. not an error)
        };

        enum class power_status_t
        {
            OnExternalPower,
            OnBattery,
            Unknown,
            NA           // No hardware support to report if on external power or battery (i.e. not an error)
        };

        
        enum class power_stat_t
        {
            BatVoltage,         // millivolts
            BatCurrent,         // milliamps. -ve is battery discharging, +ve is charging
            RemainingCapacity,  // mAh
            FullCapacity,       // mAh held by battery when full
            VbusVoltage         // millivolts
        };

        /**
         * @brief Call frequently to keep power status flags updated
         * 
         */
        virtual void loop() {};

        /**
         * @brief Get current power status
         * 
         * @return current power status - i.e. on battery, plugged, unknown or not applicable 
         */
        virtual power_status_t power_status() = 0;

        /**
         * @brief Get charging status if plugged in. Return should be ignored if plugged in.
         * 
         * @return Charging, charged, unknown or not applicable 
         */
        virtual charging_status_t charging_status() = 0;

        /**
         * @brief  Get battery/power stat
         * 
         * @param stat type to get
         * @return true  - reading/stat returned
         * @return false - stat type not available
         */
        virtual bool get_stat(int16_t* stat, power_stat_t type) = 0;

        /**
         * @brief Get battery percentage
         * 
         * @return percentage, or 255 if not known
         */
        virtual uint8_t get_battery_percentage() = 0;

        virtual ~IPowerManagement() {}
};

#endif
