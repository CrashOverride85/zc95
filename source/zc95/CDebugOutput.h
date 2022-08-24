#ifndef _CDEBUG_H
#define _CDEBUG_H

#include "CSavedSettings.h"

class CDebugOutput
{
    public:
        enum class debug_dest_t 
        {
            AUX, // 3.5mm Aux port
            ACC, // Accessory port
            OFF  // Disable debug output
        };
        
        static void set_debug_destination(debug_dest_t destination);
        static void set_debug_destination_from_settings(CSavedSettings *saved_settings);
};

#endif
