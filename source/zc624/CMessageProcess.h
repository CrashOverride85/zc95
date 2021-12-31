#ifndef _CMESSAGEPROCESS_H
#define _CMESSAGEPROCESS_H

#include "config.h"
#include "COutput.h"

class CMessageProcess
{

    public:
        struct message
        {
            uint8_t command;
            uint8_t arg0;
            uint8_t arg1;
            uint8_t arg2;
        };

        enum class command
        {
            Pulse = 0,
            SetPower = 1,
            Poll = 2,
            PowerDown = 3
        };

        CMessageProcess(COutput *output);
        ~CMessageProcess();
        void init();

        void loop();

    private:
        COutput *_output;
        void pulse(message msg);
        void set_power(message msg);

};

#endif
