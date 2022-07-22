
#include <stdio.h>
#include "pico/stdlib.h"

#include "CAnalogueCapture.h"

#include "hardware/irq.h"
#include "hardware/adc.h"
#include "hardware/dma.h"


volatile int CAnalogueCapture::_s_irq_counter1;
volatile int CAnalogueCapture::_s_irq_counter2;
uint CAnalogueCapture::_s_dma_chan1;
uint CAnalogueCapture::_s_dma_chan2;

volatile bool CAnalogueCapture::_s_buf1_ready;
volatile bool CAnalogueCapture::_s_buf2_ready;


// Called when capture_buf1 is full.
void CAnalogueCapture::s_dma_handler1()
{
  adc_select_input(ADC_CHANNEL_0);
  _s_irq_counter1++;
  _s_buf1_ready = true;
  _s_buf2_ready = false;
  // Clear the interrupt request.
  dma_hw->ints0 = 1u << _s_dma_chan1;
}

// Called when capture_buf2 is full.
void CAnalogueCapture::s_dma_handler2()
{
  adc_select_input(ADC_CHANNEL_0);
  _s_irq_counter2++;
  _s_buf2_ready = true;
  _s_buf1_ready = false;
  // Clear the interrupt request.
  dma_hw->ints0 = 1u << _s_dma_chan2;
}

void CAnalogueCapture::init()
{
    _s_irq_counter1 = 0;
    _s_irq_counter2 = 0;
    adc_gpio_init(26 + ADC_CHANNEL_0);
    adc_gpio_init(26 + ADC_CHANNEL_1);
    adc_gpio_init(26 + ADC_CHANNEL_2);
    adc_init();
    adc_select_input(ADC_CHANNEL_0);
    adc_set_round_robin(1 << ADC_CHANNEL_0 | 1 << ADC_CHANNEL_1 | 1 << ADC_CHANNEL_2);

    adc_fifo_setup (true,   // Write each completed conversion to the sample FIFO
                    true,   // Enable DMA data request (DREQ)
                    1,      // DREQ (and IRQ) asserted when at least 1 sample present
                    false,  // Don't collect error bit
                    true    // Reduce samples to 8 bits
    );

    // Determines the ADC sampling rate as a divisor of the basic
    // 48Mhz clock. Set to have 100k sps on each of the two ADC
    // channels.
    adc_set_clkdiv(2400 - 1);  // Total rate 20k sps.

    _s_dma_chan1 = dma_claim_unused_channel(true);
    _s_dma_chan2 = dma_claim_unused_channel(true);


    // Chan 1
    dma_channel_config dma_config1 = dma_channel_get_default_config(_s_dma_chan1);
    channel_config_set_transfer_data_size(&dma_config1, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_config1, false);  // ADC fifo
    channel_config_set_write_increment(&dma_config1, true);  // RAM buffer.
    // channel_config_set_write_increment(&dma_config1, true);
    // Wrap to begining of buffer. Assuming buffer is well alligned.
    channel_config_set_ring(&dma_config1, true, CAPTURE_RING_BITS);
    // Paced by ADC genered requests.
    channel_config_set_dreq(&dma_config1, DREQ_ADC);
    // When done, start the other channel.
    channel_config_set_chain_to(&dma_config1, _s_dma_chan2);
    // Using interrupt channel 0
    dma_channel_set_irq0_enabled(_s_dma_chan1, true);
    // Set IRQ handler.
    irq_set_exclusive_handler(DMA_IRQ_0, s_dma_handler1);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_configure(_s_dma_chan1, &dma_config1,
                            capture_buf1,   // dst
                            &adc_hw->fifo,  // src
                            CAPTURE_DEPTH,  // transfer count
                            true            // start immediately
    );


    // Chan 2
    dma_channel_config dma_config2 = dma_channel_get_default_config(_s_dma_chan2);
    channel_config_set_transfer_data_size(&dma_config2, DMA_SIZE_8);
    channel_config_set_read_increment(&dma_config2, false);
    channel_config_set_write_increment(&dma_config2, true);
    channel_config_set_ring(&dma_config2, true, CAPTURE_RING_BITS);
    channel_config_set_dreq(&dma_config2, DREQ_ADC);
    // When done, start the other channel.
    channel_config_set_chain_to(&dma_config2, _s_dma_chan1);
    dma_channel_set_irq1_enabled(_s_dma_chan2, true);
    irq_set_exclusive_handler(DMA_IRQ_1, s_dma_handler2);
    irq_set_enabled(DMA_IRQ_1, true);
    dma_channel_configure(_s_dma_chan2, &dma_config2,
                            capture_buf2,   // dst
                            &adc_hw->fifo,  // src
                            CAPTURE_DEPTH,  // transfer count
                            false           // Do not start immediately
    );
}

void CAnalogueCapture::start()
{
    // Start the ADC free run sampling.
    adc_run(true);
}

void CAnalogueCapture::stop()
{
    adc_run(false);
}

void CAnalogueCapture::process()
{
    if (lastc1 != _s_irq_counter1 || lastc2 != _s_irq_counter2)
    {
        uint64_t time = time_us_64() - time_last_print;
        time_last_print = time_us_64();

/*
        printf("%d, %d\tadc0=(%d, %d)\tadc1=(%d, %d)\tadc2=(%d, %d) \t%" PRId64 "us (%d ms)\n", 
        _s_irq_counter1, _s_irq_counter2,  
        capture_buf1[0], capture_buf2[0], 
        capture_buf1[1], capture_buf2[1],
        capture_buf1[2], capture_buf2[2],
        time,
        time/1000); */

        lastc1 = _s_irq_counter1;
        lastc2 = _s_irq_counter2;

        if (_s_buf1_ready)
        {
            process_buffer(capture_buf1);
            _s_buf1_ready = false;
        }

        if (_s_buf2_ready)
        {
            process_buffer(capture_buf2);
            _s_buf2_ready = false;
        }
    }
}

void CAnalogueCapture::process_buffer(uint8_t *capture_buf)
{
    for (uint x=0; x < BATTERY_ADC_READINGS; x++)
        _battery_adc_readings[x] = capture_buf[(x*3)];

    _new_battery_readings = true;
}

bool CAnalogueCapture::new_battery_readings_available()
{
    return _new_battery_readings;
}

uint8_t *CAnalogueCapture::get_battery_readings(uint8_t *readings_count)
{
    *readings_count = BATTERY_ADC_READINGS;
    _new_battery_readings = false;

    return _battery_adc_readings;
}
