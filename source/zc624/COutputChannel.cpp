#include "COutputChannel.h"
#include "pulse_gen.pio.h"
#include "hardware/adc.h"
#include <stdio.h>
#include <stdlib.h>

COutputChannel::COutputChannel(uint8_t pin_gate_a, PIO pio, uint sm, uint pio_program_offset, uint8_t adc, CDac *dac, CDac::dac_channel dac_channel)
{
    printf("COutputChannel(%d,%d)\n", pin_gate_a, sm);
    _pin_gate_a = pin_gate_a;
    _pio = pio;
    _sm  = sm;
    _adc = adc;
    _dac = dac;
    _status = status::INITIAL;
    _dac_channel = dac_channel;
    _cal_value = 0;
    _pio_program_offset = pio_program_offset;

    _dac->set_channel_value(_dac_channel, 4000); // fully switch off pfet
    pio_sm_claim(_pio, _sm);
    
    _on = false;
    _freq = 150;
    _pulse_width_pos_us = 150;
    _pulse_width_neg_us = 150;
}

COutputChannel::~COutputChannel()
{
    printf("~COutputChannel() (sm=%d)\n", _sm);
    off();
    pio_sm_set_enabled(_pio, _sm, false);
    pio_sm_unclaim(_pio, _sm);
}

int COutputChannel::cmpfunc (const void *a, const void *b)
{
   return ( *(uint32_t*)a - *(uint32_t*)b );
}

float COutputChannel::get_adc_voltage()
{
    uint32_t readings[10];
    const float conversion_factor = 3.3f / (1 << 12);

    adc_select_input(_adc);

    // get 10 readings
    for (uint8_t reading=0; reading < 10; reading++)
        readings[reading] = adc_read();

    // ignore 2 highest and 2 lowest values. get the average of the rest
    qsort(readings, 10, sizeof(uint32_t), COutputChannel::cmpfunc);
    uint32_t total=0;
    for (uint8_t reading=2; reading < 8; reading++)
        total += readings[reading];
    
    uint32_t avg = total/6;
    
    // Convert to voltage and return
    return avg * conversion_factor;
}

// Figure out at what dac value the PFET starts to turn on
bool COutputChannel::calibrate()
{
    if (_status != status::INITIAL)
    {
        return false;
    }

    float voltage = get_adc_voltage();
    if (voltage > 0.03)
    {   printf("Error - voltage is %fv before starting calibration (sm=%d)\n", voltage, _sm);
        return false;
    }

    uint16_t dac_val = 0;
    for (dac_val = 3400; dac_val > 2800; dac_val-=10)
    {
        _dac->set_channel_value(_dac_channel, dac_val);
        sleep_us(100);
        
        // switch on both N-FETs
        gpio_put(_pin_gate_a  , 1);
        gpio_put(_pin_gate_a+1, 1);

        sleep_us(50);

        voltage = get_adc_voltage();
        // printf("sm=%d: count=%d, voltage = %f\n", _sm, dac_val, voltage);
        if (voltage > 0.075)
        {
            gpio_put(_pin_gate_a  , 0);
            gpio_put(_pin_gate_a+1, 0);

            // check the voltage isn't significantly higher than expected. This shouldn't happen...
            if (voltage > 0.09)
            {
                break;
            }

            printf("calibrate for sm=%d OK: dac_val = %d, voltage = %f\n", _sm, dac_val, voltage);
            _cal_value = dac_val;
            pulse_gen_program_init(_pio, _sm, _pio_program_offset, _pin_gate_a);
            _status = status::READY;
            break;
        }

        gpio_put(_pin_gate_a  , 0);
        gpio_put(_pin_gate_a+1, 0);
        sleep_ms(5);
    }

    // switch pfet off
    _dac->set_channel_value(_dac_channel, 4000);

    if (_status != status::READY)
    {
        printf("calibrate for sm=%d FAILED! final voltage = %f, dac_value = %d (expecting 0.075v - 0.090v)\n", _sm, voltage, dac_val);
        _status = status::FAULT;
    }

    return (_status == status::READY);
}

/* Requested power will be 0-1000. There are ~2000 usable dac values for output (lower = higher power).
   At ~400 more than _cal_value the the PFET should be fully off, and at ~1600 lower it should be fully on
   (assumes an IRF9Z24NPBF)
*/
void COutputChannel::set_power(uint16_t power)
{
    if (power > 1000)
        power = 1000;

    int16_t dac_value = _cal_value + 400 - (power * 2);

    if (dac_value < 0 || dac_value > 4000)
    {
        printf("COutputChannel::set_power: ERROR - invalid dac_value calculated of %d (_cal_value=%d, power=%d)\n",
               dac_value, _cal_value, power);
        return;
    }

    if (_status == status::READY)       
    {
        _dac->set_channel_value(_dac_channel, dac_value);
    }
}

void COutputChannel::set_freq(uint16_t freq)
{
    if (freq > 1000)
        freq = 1000;

    _freq = freq;
    _freq_changed = true;
}

void COutputChannel::set_pulse_width(uint8_t pos, uint8_t neg)
{
    _pulse_width_pos_us = pos;
    _pulse_width_neg_us = neg;
}

void COutputChannel::on()
{
    if (_on)
        return;

    add_repeating_timer_us(-1000000 / _freq, s_timer_callback, this, &_timer);
    _freq_changed = false;
    _on = true;
}

void COutputChannel::off()
{
    if (!_on)
        return;
    
    _on = false;
    cancel_repeating_timer(&_timer);
}

bool COutputChannel::s_timer_callback(repeating_timer_t *rt)
{
    COutputChannel *chan = (COutputChannel*)rt->user_data;
    return chan->timer_callback(rt);
}

bool COutputChannel::timer_callback(repeating_timer_t *rt)
{
    pulse(_pulse_width_pos_us, _pulse_width_neg_us);

    if (_on)
    {
        if (_freq_changed)
        {
            //add_repeating_timer_us(-1000000 / _freq, s_timer_callback, this, &_timer);
            rt->delay_us = -1000000 / _freq;
            _freq_changed = false;
        }

        return true;
    }
    else
    {
        return false;
    }
}

void COutputChannel::pulse(uint8_t pos_us, uint8_t neg_us)
{
    if (_status != status::READY)
    {
        return;
    }

    uint8_t adjusted_len_pos = pos_us;
    uint8_t adjusted_len_neg = neg_us;
    uint32_t val = 0;
    val |= (adjusted_len_pos << 8);
    val |= adjusted_len_neg;

    if (!pio_sm_is_tx_fifo_full(_pio, _sm))
    {
        pio_sm_put_blocking(_pio, _sm, val);
    }
    else
    {
        printf("Full queue (%d)\n", _sm);
    }
}

COutputChannel::status COutputChannel::get_status()
{
    return _status;
}

// test the PFET, by starting from the highist DAC setting which should turn the PFET fully off, and
// runing through to 0, which should be fully on. Output the ADC reading for each DAC setting in a format
// that can be easily imported into a spreadsheet for graphing. It should be a nice smooth curve starting
// at around dac=3200 and leveling off around dac=1400
void COutputChannel::diag_run_dac_sweep()
{
    printf("starting test for dac_channel [%d]\n", _dac_channel);

    printf("dac_val,reading\n");
    for (int16_t dac_val = 4000; dac_val > 0; dac_val--)
    {
        _dac->set_channel_value(_dac_channel, dac_val);
        sleep_us(100);
        
        // switch on both N-FETs
        gpio_put(_pin_gate_a  , 1);
        gpio_put(_pin_gate_a+1, 1);

        sleep_us(50);

        float voltage = get_adc_voltage();

        // switch off both N-FETs
        gpio_put(_pin_gate_a  , 0);
        gpio_put(_pin_gate_a+1, 0);

        printf("%d,%f\n", dac_val, voltage);

        sleep_ms(5);
    }

    _dac->set_channel_value(_dac_channel, 4000);
}

