#ifndef _C312BOUTPUT_H
#define _C312BOUTPUT_H

#include "../../../CSavedSettings.h"
#include "../CSimpleOutputChannel.h"
#include "C312bComms.h"


class COutput312b : public CSimpleOutputChannel
{


    public:
        enum class Channel
        {
            CHAN_A,
            CHAN_B
        };



        COutput312b(CSavedSettings *saved_settings, C312bComms *comms, Channel c, CPowerLevelControl *power_level_control, uint8_t channel_id);



        void on(int16_t power);
        void off();

    private:
        C312bComms *_comms;
        Channel _channel;
    

};

#endif
