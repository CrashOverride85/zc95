#ifndef _EUTIL_H
#define _EUTIL_H

#include <inttypes.h>
#include <stddef.h>

class CInteruptableSection
{
    public:
        CInteruptableSection();
        void start();
        void end();

    private:
        bool _inital_state;
};

int i2c_write(const char *function_name, uint8_t addr, const uint8_t *src, size_t len, bool nostop);
int i2c_read (const char *function_name, uint8_t addr, uint8_t *dst      , size_t len, bool nostop);

#endif