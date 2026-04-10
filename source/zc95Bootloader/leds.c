#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "../common/zc95_config.h"
#include "leds.h"

#define LED_COUNT    6

volatile static uint32_t _led_state[LED_COUNT];

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) 
{
    return
            ((uint32_t) (r) << 16) |
            ((uint32_t) (g) << 8)  |
            (uint32_t)  (b);            
}

uint32_t get_brightness_adjusted_led_colour(uint32_t colour, int8_t brightness)
{
    uint8_t r = (colour & 0xFF0000) >> 16;
    uint8_t g = (colour & 0x00FF00) >> 8;
    uint8_t b = (colour & 0xFF);

    if (brightness < 1)
        brightness = 0;
    if (brightness > 100)
        brightness = 100;

    uint8_t r_adj = (int)((float)r * (float)brightness/(float)100);
    uint8_t g_adj = (int)((float)g * (float)brightness/(float)100);
    uint8_t b_adj = (int)((float)b * (float)brightness/(float)100);

    // For any values < 0, round up to 1 if not passed in as 0
    if (r > 0 && r_adj == 0) r_adj = 1;
    if (g > 0 && g_adj == 0) g_adj = 1;
    if (b > 0 && b_adj == 0) b_adj = 1;

    return urgb_u32(r_adj, g_adj, b_adj);
}

void leds_init()
{
    uint offset = pio_add_program(pio0, &ws2812_program);
    ws2812_program_init(pio0, 0, offset, PIN_LED, 800000, false);
    
    leds_clear();
}

void leds_clear()
{
    // Clear LEDs / switch all off
    for (uint8_t n = 0; n < LED_COUNT ; n++)
        pio_sm_put_blocking(pio0, 0, 0);
}

void leds_show_percent(uint8_t percent, uint32_t colour)
{
    uint8_t led_count = percent / (100 / LED_COUNT);
    if (percent >= 100)
        led_count = LED_COUNT;

    if (led_count == 0)
        led_count = 1;

    for (uint8_t n = 0; n < LED_COUNT ; n++)
    {
        uint32_t pixel_rgb = 0;
        
        if (n < led_count)
            pixel_rgb = get_brightness_adjusted_led_colour(colour, 10);

        pio_sm_put_blocking(pio0, 0, pixel_rgb << 8u);
    }
}

void leds_show_update_error()
{
      // Set right and left most LEDs to red, switch the rest off, 

    uint32_t colour = get_brightness_adjusted_led_colour(COLOUR_RED, 2);

    pio_sm_put_blocking(pio0, 0, colour << 8u);
    pio_sm_put_blocking(pio0, 0, 0 << 8u);
    pio_sm_put_blocking(pio0, 0, 0 << 8u);
    pio_sm_put_blocking(pio0, 0, 0 << 8u);
    pio_sm_put_blocking(pio0, 0, 0 << 8u);
    pio_sm_put_blocking(pio0, 0, colour << 8u);   
}

void led_startup()
{
    // Set right and left most LEDs to purple, switch the rest off, 

    uint32_t colour = get_brightness_adjusted_led_colour(COLOUR_PURPLE, 2);

    pio_sm_put_blocking(pio0, 0, colour << 8u);
    pio_sm_put_blocking(pio0, 0, 0 << 8u);
    pio_sm_put_blocking(pio0, 0, 0 << 8u);
    pio_sm_put_blocking(pio0, 0, 0 << 8u);
    pio_sm_put_blocking(pio0, 0, 0 << 8u);
    pio_sm_put_blocking(pio0, 0, colour << 8u); 
} 