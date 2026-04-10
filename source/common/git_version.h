// see https://jonathanhamberg.com/post/cmake-embedding-git-hash/

#ifndef GIT_VERSION_H
#define GIT_VERSION_H

#include <inttypes.h>

typedef struct
{
    // Marker to try and make sure the wrong firmware doesn't get loaded into flash
    uint32_t magic;

    // Version of this struct
    uint32_t struct_version;

    // Firmware version, more-or-less the result of a "git describe --always --dirty=-x"
    char firmware_version[32];

    // For zc624 the major version. For zc95 firmware, the major version of the zc624 firmware required. Not used (set to 0) for both bootloaders
    uint8_t fw624_major;

    // For zc624 the minor version. For zc95 firmware, the minimum minor version of the zc624 firmware required. Not used (set to 0) for both bootloaders
    uint8_t fw624_minor;
} firmware_info_t;


extern const firmware_info_t firmware_info;

#endif // GIT_VERSION_H
