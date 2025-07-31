#ifndef _IHAL_H
#define _IHAL_H

#include <inttypes.h>

#include "../PowerManagement/IPowerManagement.h"
#include "../PortExpanders/CExtInputPortExp.h"
#include "../PortExpanders/CMainBoardPortExp.h"
#include "../PortExpanders/EExtInputPort.h"
#include "../FrontPanel/CFrontPanel.h"
#include "../ZcTypes.h"

/**
 * @brief Try and abstract the differences between MK1 and MK2 ZC95's
 * 
 */
class IHal
{
    public:
        virtual IPowerManagement* power_management() = 0;
        virtual CFrontPanel* front_panel() = 0;
        virtual front_panel_version_t front_panel_version() = 0;

        virtual void set_backlight(bool on) = 0;
        virtual zc95_version_t hardware_version() = 0;
        virtual hw_variant_t hardware_variant() = 0;
        
        virtual void loop() = 0;
        
        // accessory port
        virtual void acc_port_reset() = 0;
        virtual void acc_port_set_io_port_state(ExtInputPort output, bool high) = 0;

        // audio
        virtual void audio_input_enable(bool enable) = 0;
        virtual void mic_preamp_enable(bool enable) = 0;
        virtual void mic_power_enable(bool enable) = 0;
};

#endif
