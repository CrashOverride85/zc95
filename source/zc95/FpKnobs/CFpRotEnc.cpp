#include "CFpRotEnc.h"
#include "../globals.h"
#include "../CUtil.h"

#include "hardware/gpio.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Front pannel which has 5x rotary encoders connected to two I2C port expanders.
 *
 * 2021/09/24: Not sure if this still works, need to check that this version of the
 *             front pannel still behaves as expected (TODO)
 */

#define PORT_EXP_1_ADDR 0x24 // U1
#define PORT_EXP_2_ADDR 0x26 // U2

// ROT1 = channel 4 (right most control when viewed from front)
// ROT2 = channel 3
// ROT3 = channel 2
// ROT4 = channel 1 
// ROT5 = adjust

// PORT_EXP_1
#define ROT1_A   0
#define ROT1_B   1

#define ROT2_A   2
#define ROT2_B   3

#define ROT3_A   4
#define ROT3_B   5

#define ROT4_A   6
#define ROT4_B   7

// PORT_EXP_2
#define NC       0
#define ROT1_BUT 1
#define ROT2_BUT 2
#define ROT3_BUT 3
#define ROT4_BUT 4
#define ROT5_BUT 5
#define ROT5_A   6
#define ROT5_B   7

CFpRotEnc::CFpRotEnc(CSavedSettings *saved_settings)
{
    memset(_power_level, 0, sizeof(_power_level));
    _adjust_value = 0;
    _saved_settings = saved_settings;

 //   write_port_expander(port_exp::U1, 0xFF);
}

void CFpRotEnc::process(bool always_update)
{
    if (_interrupt_U1 || always_update)
    {
        _interrupt_U1 = false;
        process_u1();
    }

    if (_interrupt_U2 || always_update)
    {
        _interrupt_U2 = false;
        process_u2();
    }
}

void CFpRotEnc::process_u1()
{
    uint8_t buffer;
    buffer = read_port_expander(port_exp::U1);

    if (_last_read[port_exp::U1] != buffer)
    {
        rot_encoder_process(ROT_ENCODER_CHANNEL1, buffer, ROT4_A, ROT4_B);
        rot_encoder_process(ROT_ENCODER_CHANNEL2, buffer, ROT3_A, ROT3_B);
        rot_encoder_process(ROT_ENCODER_CHANNEL3, buffer, ROT2_A, ROT2_B);
        rot_encoder_process(ROT_ENCODER_CHANNEL4, buffer, ROT1_A, ROT1_B);
        update_power_levels();
        _last_read[port_exp::U1] = buffer;
    }
}

void CFpRotEnc::process_u2()
{
    uint8_t buffer;
    buffer = read_port_expander(port_exp::U2);

    if (_last_read[port_exp::U2] != buffer)
    {
        rot_encoder_process(ROT_ENCODER_ADJUST, buffer, ROT5_A, ROT5_B);

        if (abs(_adjust_value < 100))
            _adjust_value += _rot_encoder[ROT_ENCODER_ADJUST].get_rotary_encoder_change();

        _last_read[port_exp::U2] = buffer;
    }
}


void CFpRotEnc::interupt(port_exp exp)
{
    if (exp == port_exp::U1)
        _interrupt_U1 = true;
    else if (exp == port_exp::U2)
        _interrupt_U2 = true;
}

uint16_t CFpRotEnc::get_channel_power_level(uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return 0;

    return _power_level[channel];
}

int8_t CFpRotEnc::get_adjust_control_change()
{
    int8_t retval = _adjust_value;
    _adjust_value = 0;
    return retval;
}

void CFpRotEnc::update_power_levels()
{
    update_power_level(ROT_ENCODER_CHANNEL1, 0);
    update_power_level(ROT_ENCODER_CHANNEL2, 1);
    update_power_level(ROT_ENCODER_CHANNEL3, 2);
    update_power_level(ROT_ENCODER_CHANNEL4, 3);
}

// channel is 0-3
void CFpRotEnc::update_power_level(rot_encoder rot, uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return;

    int8_t rot_change = _rot_encoder[rot].get_rotary_encoder_change();

    if (rot_change)
    {
        printf("rot = %d, change = %d\n", rot, rot_change);
        
        if (rot_change > 0)
        {
            _power_level[channel] += _saved_settings->get_power_step_interval();
        }
        else
        {
            _power_level[channel] -= _saved_settings->get_power_step_interval();
        }
       
        if (_power_level[channel] < 0)
            _power_level[channel] = 0;

        if (_power_level[channel] > MAX_POWER_LEVEL)
            _power_level[channel] = MAX_POWER_LEVEL;
    }
}

void CFpRotEnc::rot_encoder_process(rot_encoder rot, uint8_t port_exp_read, uint8_t a_pos, uint8_t b_pos)
{
    uint8_t a = (port_exp_read & (1 << a_pos) ? 1 : 0);
    uint8_t b = (port_exp_read & (1 << b_pos ) ? 1 : 0);

    if (rot == rot_encoder::ROT_ENCODER_CHANNEL1)
    {
        printf("a = %d, b = %d\n", a , b);
    }

    _rot_encoder[rot].process(a, b);
}

uint8_t CFpRotEnc::read_port_expander(port_exp exp)
{
    uint8_t address = 0;

    switch (exp)
    {
        case port_exp::U1:
            address = PORT_EXP_1_ADDR;
            break;
        
        case port_exp::U2:
            address = PORT_EXP_2_ADDR;
            break;

        default:
            return 0;
    }

    uint8_t buffer[1];
    
    int retval = i2c_read(__func__, address, buffer, 1, false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CFpRotEnc::read_port_expander i2c read error!\n");
      return 0;
    }

    return buffer[0];
}

void CFpRotEnc::write_port_expander(port_exp exp, uint8_t val)
{
    uint8_t address = 0;

    switch (exp)
    {
        case port_exp::U1:
            address = PORT_EXP_1_ADDR;
            break;
        
        case port_exp::U2:
            address = PORT_EXP_2_ADDR;
            break;

        default:
            return;
    }

    uint8_t buffer[1];
    int retval = i2c_write(__func__, address, buffer, 1, false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
        printf("CFpRotEnc::write_port_expander i2c write error!\n");
        return;
    }

    return;
}
