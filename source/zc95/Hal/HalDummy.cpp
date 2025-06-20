#include "HalDummy.h"
#include "../HwCheck/CDetermineHardwareVersion.h"

/* Dummy HAL for when hardware version can't be determined */

HalDummy::HalDummy()
{   
    gpio_init(PIN_MK1_DISP_RST);
    gpio_set_dir(PIN_MK1_DISP_RST, GPIO_OUT);
    gpio_put(PIN_MK1_DISP_RST, true); // RST is active low
}

HalDummy::~HalDummy()
{
}

void HalDummy::loop()
{

}

IPowerManagement* HalDummy::power_management() 
{
    return NULL;
}

CExtInputPortExp* HalDummy::external_input_port_exp()
{
    return NULL;
}

CMainBoardPortExp* HalDummy::mainboard_port_exp()
{
    return NULL;
}

CFrontPanel* HalDummy::front_panel()
{
    return NULL;
}

void HalDummy::set_backlight(bool on)
{
    
}

zc95_version_t HalDummy::hardware_version()
{
    return zc95_version_t::UNKNOWN;
}

void HalDummy::acc_port_reset()
{
    
}

void HalDummy::acc_port_set_io_port_state(ExtInputPort output, bool high)
{
    
}

void HalDummy::audio_input_enable(bool enable)
{
    
}

void HalDummy::mic_preamp_enable(bool enable)
{
    
}

void HalDummy::mic_power_enable(bool enable)
{
    
}

front_panel_version_t HalDummy::front_panel_version()
{
    return front_panel_version_t::UNKNOWN;
}
