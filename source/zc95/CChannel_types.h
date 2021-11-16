#ifndef _CCHANNELTYPES_H
#define _CCHANNELTYPES_H


#include <string>
#include <inttypes.h>

class CChannel_types
{

    public:

        enum class channel_type
        {
            CHANNEL_NONE     = 0,
            CHANNEL_INTERNAL = 1,
            CHANNEL_COLLAR   = 2,
            CHANNEL_312      = 3
        };

        static std::string get_channel_name(channel_type type, uint8_t index);
};

#endif
