#ifndef _EEPROM_H
#define _EEPROM_H

#include <stdint.h>
#include <stdbool.h>


#include "../common/zc95_config.h"

uint8_t get_eeprom_flag();
bool   set_eeprom_flag(uint8_t val);

#endif