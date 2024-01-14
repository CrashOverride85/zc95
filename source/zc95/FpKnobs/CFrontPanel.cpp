#include "CFrontPanel.h"
#include <string.h>
#include "../globals.h"
#include "../CUtil.h"

#define ROT_A   6
#define ROT_B   7

/*
 * Front panel which has 4x potentiometers connected to an I2C ADC, 
 * and 1x rotary encoder connected to an I2C port expander
 */

CFrontPanel::CFrontPanel(CSavedSettings *saved_settings)
{
    memset(_power_level, 0, sizeof(_power_level));
    _last_port_exp_read = 0;
    _adjust_value = 0;
    
    // The very first time the ADC is read, the first channel seems to have an invalid value. So read it now, 
    // so when it's read next time as usual, it'll return something sensible.
    // This is just to stop the channel 1 bar graph briefly (fraction of a second) showing ~50% power on startup
    read_adc();
}

void CFrontPanel::interrupt (port_exp exp)
{
    _interrupt = true;
    _interrupt_time = time_us_32();

    if (gInteruptable)
        process(false);
}

void CFrontPanel::process(bool always_update)
{
    if (_interrupt || always_update)
    {
        if (_interrupt)
        {
            //printf("delay=%lu us\n", time_us_32() - _interrupt_time);
            _interrupt = false;
        }
        
        if (always_update)
        {
            read_adc();
        }
       
        uint8_t buffer;
        buffer = read_port_expander();

        if (_last_port_exp_read != buffer)
        {
            rot_encoder_process(buffer, ROT_A, ROT_B);

            if (abs(_adjust_value < 100))
                _adjust_value += _rot_encoder.get_rotary_encoder_change();

            _last_port_exp_read = buffer;
        }
    }
}

uint16_t CFrontPanel::get_channel_power_level(uint8_t channel)
{
    if (channel >= MAX_CHANNELS)
        return 0;

    return _power_level[channel];
}


int8_t CFrontPanel::get_adjust_control_change()
{
    int8_t retval = _adjust_value;
    _adjust_value = 0;
    return retval;
}

 
void CFrontPanel::read_adc()
{
    uint8_t command = 0x04;

    int bytes_written = i2c_write(__func__, ADC_ADDR, &command, 1, false);
    if (bytes_written != 1)
    {
        printf("CFrontPanel::read_adc() write failed! i2c bytes_written = %d\n", bytes_written);
        return;
    }

    uint8_t buffer[5] = {0};
    int retval = i2c_read(__func__, ADC_ADDR, buffer, sizeof(buffer), false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CFrontPanel::read_adc() i2c read error!\n");
      return;
    }

    //printf("ADC: %d\t %d\t %d\t %d\n", buffer[1],buffer[2],buffer[3],buffer[4]);
    // buf : chan
    // 1   : 1
    // 2   : 4
    // 3   : 3
    // 4   : 2

    _power_level[0] = (float)1000 - (float)buffer[1] * 3.91;
    _power_level[1] = (float)1000 - (float)buffer[4] * 3.91;
    _power_level[2] = (float)1000 - (float)buffer[3] * 3.91;
    _power_level[3] = (float)1000 - (float)buffer[2] * 3.91;
}

void CFrontPanel::rot_encoder_process(uint8_t port_exp_read, uint8_t a_pos, uint8_t b_pos)
{
    uint8_t a = (port_exp_read & (1 << a_pos) ? 1 : 0);
    uint8_t b = (port_exp_read & (1 << b_pos ) ? 1 : 0);

    _rot_encoder.process(a, b);
}

uint8_t CFrontPanel::read_port_expander()
{
    uint8_t buffer[1];
    
    int retval = i2c_read(__func__, FP_0_1_PORT_EXP_ADDR, buffer, 1, false);
    if (retval == PICO_ERROR_GENERIC || retval == PICO_ERROR_TIMEOUT)
    {
      printf("CFrontPanel::read_port_expander i2c read error!\n");
      return 0;
    }

    return buffer[0];
}
