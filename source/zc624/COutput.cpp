#include "config.h"
#include "COutput.h"
#include <stdio.h>

#define CHANNEL_COUNT 4

COutput::COutput(PIO pio, CI2cSlave *i2c_slave)
{
    printf("COutput()\n");
    _pio = pio;
    _pio_program_offset = pio_add_program(_pio, &pulse_gen_program);
    _i2c_slave = i2c_slave;
    _pulse_queue = new CPulseQueue(i2c_slave);

    setup_gpio(PIN_9V_ENABLE);
    setup_gpio(PIN_CHAN1_GATE_A);
    setup_gpio(PIN_CHAN1_GATE_B);
    setup_gpio(PIN_CHAN2_GATE_A);
    setup_gpio(PIN_CHAN2_GATE_B);
    setup_gpio(PIN_CHAN3_GATE_A);
    setup_gpio(PIN_CHAN3_GATE_B);
    setup_gpio(PIN_CHAN4_GATE_A);
    setup_gpio(PIN_CHAN4_GATE_B);

    _channel[0] = new COutputChannel(PIN_CHAN1_GATE_A, _pio, 0, _pio_program_offset, 0, &_dac, CDac::dac_channel::A, _pulse_queue);
    _channel[1] = new COutputChannel(PIN_CHAN2_GATE_A, _pio, 1, _pio_program_offset, 0, &_dac, CDac::dac_channel::B, _pulse_queue);
    _channel[2] = new COutputChannel(PIN_CHAN3_GATE_A, _pio, 2, _pio_program_offset, 1, &_dac, CDac::dac_channel::C, _pulse_queue);
    _channel[3] = new COutputChannel(PIN_CHAN4_GATE_A, _pio, 3, _pio_program_offset, 1, &_dac, CDac::dac_channel::D, _pulse_queue);

    gpio_put(PIN_9V_ENABLE, 1);
    sleep_ms(100); // wait for 9v supply to stabilize

    for (int chan=0; chan < 4; chan++)
        _channel[chan]->calibrate();

    bool all_ready = true;
    for (int chan=0; chan < 4; chan++)
    {
        COutputChannel::status chan_status = _channel[chan]->get_status();
        if (chan_status == COutputChannel::status::READY)
        {
            _i2c_slave->set_value(((uint8_t)CI2cSlave::reg::Chan0Status)+chan, CI2cSlave::status::Ready);
        }
        else
        {
            _i2c_slave->set_value(((uint8_t)CI2cSlave::reg::Chan0Status)+chan, CI2cSlave::status::Fault);
            all_ready = false;
        }
    }

    if (all_ready)
    {
        printf("Calibration success\n");
        _i2c_slave->set_value((uint8_t)CI2cSlave::reg::OverallStatus, CI2cSlave::status::Ready);
        gpio_put(PIN_9V_ENABLE, 1);
    }
    else
    {
        printf("One or more chanel failed calibration, not enabling power.\n");
        _i2c_slave->set_value((uint8_t)CI2cSlave::reg::OverallStatus, CI2cSlave::status::Fault);
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

    if (_pulse_queue != NULL)
    {
        delete _pulse_queue;
        _pulse_queue = NULL;
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
    if (_pulse_queue->get_queued_pulse(&sm, &pos, &neg))
    {
        if (!is_channel_valid(sm))
            return;

        _channel[sm]->do_pulse(pos, neg);
    }
}

uint8_t COutput::get_channel_led_state()
{
    uint8_t state = 0;
    
    for (uint8_t chan = 0; chan < CHANNEL_COUNT; chan++)
    {
        if (_channel[chan]->get_channel_led())
        {
            state |= (1 << chan);
        }
    }
    
    return state;
}

void COutput::power_down()
{
    printf("COutput::power_down()\n");
    gpio_put(PIN_9V_ENABLE, 0);
    
    // Could probably do with an extra status. But as there's currently no return 
    // from PowerDown, it's good enough for now
    _i2c_slave->set_value((uint8_t)CI2cSlave::reg::OverallStatus, CI2cSlave::status::Fault); 

    for (int chan=0; chan < CHANNEL_COUNT; chan++)
    {
        _channel[chan]->off();
        _channel[chan]->set_power(0);
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
