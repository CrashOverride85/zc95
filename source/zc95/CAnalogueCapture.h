#ifndef _CANALOGUECAPTURE_H
#define _CANALOGUECAPTURE_H

#include <inttypes.h>
#include <stdio.h>

// DMA/ADC code copied/adapted from https://forums.raspberrypi.com/viewtopic.php?p=1861895#p1861895

#define ADC_CHANNEL_0 0
#define ADC_CHANNEL_1 1
#define ADC_CHANNEL_2 2
#define CAPTURE_DEPTH 1024
#define CAPTURE_RING_BITS 10

#define BATTERY_ADC_READINGS 10

class CAnalogueCapture
{
    public:
        void init();
        void start();
        void stop();
        void process();
        bool new_battery_readings_available();
        uint8_t *get_battery_readings(uint8_t *readings_count);

    private:
        void process_buffer(uint8_t *capture_buf);

        static void s_dma_handler1();
        static void s_dma_handler2();

        uint8_t capture_buf1[CAPTURE_DEPTH] __attribute__((aligned(2048)));
        uint8_t capture_buf2[CAPTURE_DEPTH] __attribute__((aligned(2048)));
        
        static uint _s_dma_chan1;
        static uint _s_dma_chan2;
        static volatile int _s_irq_counter1;
        static volatile int _s_irq_counter2;
        static volatile bool _s_buf1_ready;
        static volatile bool _s_buf2_ready;

        uint8_t _battery_adc_readings[BATTERY_ADC_READINGS] = {0};

        int lastc1 = 0;
        int lastc2 = 0;
        uint64_t time_last_print=0;
        bool _new_battery_readings = false;
};

#endif
