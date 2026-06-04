#include "../common/zc624_config.h"
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
    reset_chanel_triphase(0);
    reset_chanel_triphase(1);
    reset_chanel_triphase(2);
    reset_chanel_triphase(3);
    _channel_isolation_last_value = _i2c_slave->get_value(CI2cSlave::reg::ChannelIsolation);

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
            _i2c_slave->set_value(((uint8_t)CI2cSlave::reg::Chan0Status)+chan, ZC624_OVERALL_STATUS_READY);
        }
        else
        {
            _i2c_slave->set_value(((uint8_t)CI2cSlave::reg::Chan0Status)+chan, ZC624_OVERALL_STATUS_FAULT);
            all_ready = false;
        }
    }

    if (all_ready)
    {
        printf("Calibration success\n");
        _i2c_slave->set_value((uint8_t)CI2cSlave::reg::OverallStatus, ZC624_OVERALL_STATUS_READY);
        gpio_put(PIN_9V_ENABLE, 1);
    }
    else
    {
        printf("One or more chanel failed calibration, not enabling power.\n");
        _i2c_slave->set_value((uint8_t)CI2cSlave::reg::OverallStatus, ZC624_OVERALL_STATUS_FAULT);
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

void COutput::reset_chanel_triphase(uint8_t chan)
{
    if (!is_channel_valid(chan))
        return;

    _channel_triphase[chan].linked_channel = 0xFF;
    _channel_triphase[chan].offset_percent = 0xFF;
}

void COutput::pulse(uint8_t channel, uint8_t pos_us, uint8_t neg_us, uint64_t delay_until_us)
{
    if (!is_channel_valid(channel))
        return;

    _channel[channel]->queue_pulse(pos_us, neg_us, delay_until_us);
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

    unlink_if_linked(channel);
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

    bool channel_isolation_on = _i2c_slave->get_value(CI2cSlave::reg::ChannelIsolation);
    if (_channel_isolation_last_value != channel_isolation_on)
    {
        reset_chanel_triphase(0);
        reset_chanel_triphase(1);
        reset_chanel_triphase(2);
        reset_chanel_triphase(3);
        _channel_isolation_last_value = channel_isolation_on;
    }

    if (_pulse_queue->get_queued_pulse(&sm, &pos, &neg))
    {
        if (!is_channel_valid(sm))
            return;

        _channel[sm]->do_pulse(pos, neg);

        if (!channel_isolation_on && is_channel_valid(_channel_triphase[sm].linked_channel) && _channel_triphase[sm].offset_percent <= 100)
        {           
            uint16_t pulse_duration_us = pos + neg;
            uint16_t delay_us = (float)_channel_triphase[sm].offset_percent * ((float)pulse_duration_us/(float)100); // max delay = 512 us
            
            pulse(_channel_triphase[sm].linked_channel, pos, neg, time_us_64() + delay_us);
        }
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

    state |= (1 << 5);
    
    return state;
}

void COutput::sync_chanel(uint8_t channel_lead, uint8_t channel_linked, uint8_t offset_percent)
{
    bool channel_isolation_on = _i2c_slave->get_value(CI2cSlave::reg::ChannelIsolation);
    if (!is_channel_valid(channel_lead) || channel_lead==channel_linked || offset_percent > 100 || channel_isolation_on)
    {
        printf("Rejecting SyncChanel message\n");
        return;
    }

    // Don't allow a linked channel to be linked to another. E.g. if 1 is linked to 2, don't allow 
    // 2 to be linked to anything. This is mostly to prevent loops (e.g. 1 -> 2 -> 1). We could be 
    // smarter in the future and explicitly look for (and stop) creating loops, but not yet.
    for (uint8_t chan=0; chan++; chan < 4)
    {
        if (_channel_triphase[chan].linked_channel == channel_lead)
        {
            printf("Rejecting SyncChanel message: channel already linked\n");
            return;
        }
    }

    if (!is_channel_valid(channel_linked))
    {
        reset_chanel_triphase(channel_lead);
        return;
    }

    off(channel_linked);

    _channel_triphase[channel_lead].linked_channel = channel_linked;
    _channel_triphase[channel_lead].offset_percent = offset_percent;
}

bool COutput::is_linked_channel(uint8_t channel)
{
    for (uint8_t c=0; c < 4; c++)
    {
        if (_channel_triphase[c].linked_channel == channel)
            return true;
    }

    return false;
}

void COutput::unlink_if_linked(uint8_t channel)
{
    for (uint8_t c=0; c < 4; c++)
    {
        if (_channel_triphase[c].linked_channel == channel)
            reset_chanel_triphase(c);
    }
}

void COutput::power_down()
{
    printf("COutput::power_down()\n");
    gpio_put(PIN_9V_ENABLE, 0);
    
    // Could probably do with an extra status. But as there's currently no return 
    // from PowerDown, it's good enough for now
    _i2c_slave->set_value((uint8_t)CI2cSlave::reg::OverallStatus, ZC624_OVERALL_STATUS_FAULT); 

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
