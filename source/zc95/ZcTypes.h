#ifndef _ZCTYPES_H
#define _ZCTYPES_H

#include <stdint.h>

enum class front_panel_version_t
{
    UNKNOWN,
    v0_1,
    v0_2
};

enum class zc95_version_t
{
    UNKNOWN,
    MKI,
    MKII
};

enum class hw_variant_t
{
    NA  , // Not a MKII
    V2_0, // Main board PCB versions 2.0 & 2.1
    V2_2  // Main board PCB versions >= 2.2
};

typedef uint64_t time_us_t;

#endif
