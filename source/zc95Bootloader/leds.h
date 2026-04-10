#ifndef _LEDS_H_
#define _LEDS_H_

#include <pico/stdio.h>

#define COLOUR_PURPLE   8388736
#define COLOUR_BLUE         255
#define COLOUR_RED     16711680
#define COLOUR_YELLOW  16776960

void leds_init();
void leds_clear();
void leds_show_percent(uint8_t percent, uint32_t colour);
void leds_show_update_error();
void led_startup();

#endif
