#include "config.h"
#include "COutput.h"
#include <stdio.h>

#define CHANNEL_COUNT 4

COutput::COutput(PIO pio)
{
    printf("COutput()\n");
    _pio = pio;
    _pio_program_offset = pio_add_program(_pio, &pulse_gen_program);

    setup_gpio(PIN_9V_ENABLE);
    setup_gpio(PIN_CHAN1_GATE_A);
    setup_gpio(PIN_CHAN1_GATE_B);
    setup_gpio(PIN_CHAN2_GATE_A);
    setup_gpio(PIN_CHAN2_GATE_B);
    setup_gpio(PIN_CHAN3_GATE_A);
    setup_gpio(PIN_CHAN3_GATE_B);
    setup_gpio(PIN_CHAN4_GATE_A);
    setup_gpio(PIN_CHAN4_GATE_B);

    _channel[0] = new COutputChannel(PIN_CHAN1_GATE_A, _pio, 0, _pio_program_offset, 0, &_dac, CDac::dac_channel::A, &_pulse_queue);
    _channel[1] = new COutputChannel(PIN_CHAN2_GATE_A, _pio, 1, _pio_program_offset, 0, &_dac, CDac::dac_channel::B, &_pulse_queue);
    _channel[2] = new COutputChannel(PIN_CHAN3_GATE_A, _pio, 2, _pio_program_offset, 1, &_dac, CDac::dac_channel::C, &_pulse_queue);
    _channel[3] = new COutputChannel(PIN_CHAN4_GATE_A, _pio, 3, _pio_program_offset, 1, &_dac, CDac::dac_channel::D, &_pulse_queue);

    gpio_put(PIN_9V_ENABLE, 1);
    sleep_ms(100); // wait for 9v suppy to stabalise

    for (int chan=0; chan < 4; chan++)
        _channel[chan]->calibrate();

    bool all_ready = true;
    for (int chan=0; chan < 4; chan++)
        all_ready &= (_channel[chan]->get_status() == COutputChannel::status::READY);

    if (all_ready)
    {
        printf("Calibration success\n");
        gpio_put(PIN_9V_ENABLE, 1);
    }
    else
    {
        printf("One or more chanel failed calibration, not enabling power.\n");
        gpio_put(PIN_9V_ENABLE, 0);
    }
}

COutput::~COutput()
{
    printf("~COutput()\n");

    for (uint8_t x=0; x < CHANNEL_COUNT; x++)
    {
        if (_channel[x])
        {
            delete _channel[x];
            _channel[x] = NULL;
        }
    }
}

void COutput::pulse(uint8_t channel, uint8_t pos_us, uint8_t neg_us)
{
    if (!is_channel_valid(channel))
        return;

    _channel[channel]->queue_pulse(pos_us, neg_us);
}

void COutput::set_power(uint8_t channel, uint16_t power)
{
    if (!is_channel_valid(channel))
        return;

    _channel[channel]->set_power(power);
}

void COutput::set_freq(uint8_t channel, uint16_t freq)
{
    if (!is_channel_valid(channel))
        return;

    _channel[channel]->set_freq(freq);
}

void COutput::set_pulse_width(uint8_t channel, uint8_t pos, uint8_t neg)
{
    if (!is_channel_valid(channel))
        return;

    _channel[channel]->set_pulse_width(pos, neg);
}

void COutput::on(uint8_t channel)
{
    if (!is_channel_valid(channel))
        return;

    _channel[channel]->on();
}

void COutput::off(uint8_t channel)
{
    if (!is_channel_valid(channel))
        return;

    _channel[channel]->off();
}

void COutput::loop()
{
    uint sm = 0;
    uint8_t pos = 0;
    uint8_t neg = 0;
    if (_pulse_queue.get_queued_pulse(&sm, &pos, &neg))
    {
        if (!is_channel_valid(sm))
            return;

        _channel[sm]->do_pulse(pos, neg);
    }
}

void COutput::setup_gpio(uint8_t pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

bool COutput::is_channel_valid(uint8_t channel)
{
    if (channel >= CHANNEL_COUNT)
        return false;

    if (_channel[channel] == NULL)
        return false;

    return true;
}
