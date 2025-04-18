#ifndef _ZCTYPES_H
#define _ZCTYPES_H

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

typedef uint64_t time_us_t;

#endif
