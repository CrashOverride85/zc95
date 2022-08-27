#ifndef _CMCP4651_H
#define _CMCP4651_H

#include <inttypes.h>
#include "../config.h"

class CMCP4651 {
	
  public:
    CMCP4651(uint8_t addr = AUDIO_DIGIPOT_ADDR);
	void set_val(uint8_t pot, uint16_t val);

    private:
        uint8_t _address;
};

#endif
