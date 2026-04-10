#include "zc_debug.h"

// When the upload utility is being used, inhibit extra messages sent on 
// the 3.5mm serial connection it uses that would confuse it
bool _inhibit_35mm_messages = false; 

struct buffered_serial_ctx _bs_ctx;
