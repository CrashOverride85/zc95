#include <inttypes.h>
#include "hardware/i2c.h"

class i2c_scan
{
    public:
        static void scan(i2c_inst_t *i2c);

    private:
        static bool reserved_addr(uint8_t addr);
};
