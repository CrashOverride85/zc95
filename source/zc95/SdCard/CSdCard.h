#ifndef _CSDCARD_H
#define _CSDCARD_H

#include <inttypes.h>
#include <stdio.h>
#include <string>

#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
#include "hw_config.h"

class CSdCard
{
    public:
        CSdCard();
        ~CSdCard();

        void mount();
        void dir_list();

        bool write_file(std::string filename, const char* contents, size_t len);


    private:
    
       
};

#endif
