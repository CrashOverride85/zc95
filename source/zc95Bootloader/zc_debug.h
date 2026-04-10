#ifndef _ZC_DEBUG_H_
#define _ZC_DEBUG_H_

#include "buffered_serial.h"


extern struct buffered_serial_ctx _bs_ctx;
extern bool _inhibit_35mm_messages; // When the upload utility is being used, inhibit extra messages that confuse it

#endif
