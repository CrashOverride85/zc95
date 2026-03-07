#ifndef _IHAMK1L_H
#define _IHALDUMMY_H

#include <inttypes.h>

#include "IHal.h"
#include "../CAnalogueCapture.h"
#include "../CSavedSettings.h"
#include "../PortExpanders/PCF8574.h"
#include "PowerManagement/CPowerManagementMk1.h"

class HalDummy : public IHal
{
    public:
        HalDummy();
        ~HalDummy();

        IPowerManagement* power_management();
        CExtInputPortExp* external_input_port_exp();
        CMainBoardPortExp* mainboard_port_exp();
        CFrontPanel* front_panel();
        front_panel_version_t front_panel_version();

        void set_backlight(bool on);
        zc95_version_t hardware_version();
        hw_variant_t hardware_variant() {return hw_variant_t::NA;}

        void loop();

        void acc_port_reset();
        void acc_port_set_io_port_state(ExtInputPort output, bool high);

        void audio_input_enable(bool enable);
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);
};

#endif
