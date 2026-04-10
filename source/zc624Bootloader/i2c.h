#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include "../common/zc95_config.h"

void i2c_start();
int i2c_write(const char *function_name, uint8_t addr, const uint8_t *src, size_t len, bool nostop);
int i2c_read (const char *function_name, uint8_t addr, uint8_t *dst      , size_t len, bool nostop);
