#ifndef _CDETERMINEHARDWAREVERSION_H
#define _CDETERMINEHARDWAREVERSION_H

#include <stdio.h>

#include "../ZcTypes.h"

class CDetermineHardwareVersion
{
    public:
        static zc95_version_t get_hardware_version();
        static front_panel_version_t get_front_panel_version();
};

#endif
