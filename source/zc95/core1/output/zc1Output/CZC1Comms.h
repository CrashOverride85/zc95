#ifndef _CZC1COMMS_H
#define _CZC1COMMS_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

class CZC1Comms
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

        CZC1Comms(spi_inst_t *spi);
        ~CZC1Comms();

        void send_message(message msg);

    private:
        spi_inst_t *_spi;


};


#endif
