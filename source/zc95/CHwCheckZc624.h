#ifndef _CHWCHECKZC624_H
#define _CHWCHECKZC624_H

class CHwCheckZc624
{
    public:
        enum class reg
        {
            TypeLow        = 0x00,
            TypeHigh       = 0x01,
            VersionMajor   = 0x02,
            VersionMinor   = 0x03,

            OverallStatus  = 0x0F,
            Chan0Status    = 0x10,
            Chan1Status    = 0x11,
            Chan2Status    = 0x12,
            Chan3Status    = 0x13
        };

        enum status
        {
            Startup   = 0x00,
            Ready     = 0x01,
            Fault     = 0x02
        };
};

#endif
