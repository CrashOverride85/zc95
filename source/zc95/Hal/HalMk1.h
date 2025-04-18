#ifndef _IHAMK1L_H
#define _IHALMK1_H

#include <inttypes.h>

#include "IHal.h"
#include "../CAnalogueCapture.h"
#include "../CSavedSettings.h"
#include "../PortExpanders/PCF8574.h"
#include "PowerManagement/CPowerManagementMk1.h"

class HalMk1 : public IHal
{
    public:
        HalMk1(CLedControl *led, CRoutineOutput **routine_output, CAnalogueCapture* analogue_capture, CSavedSettings **saved_settings);
        ~HalMk1();

        IPowerManagement* power_management();
        CExtInputPortExp* external_input_port_exp();
        CMainBoardPortExp* mainboard_port_exp();
        CFrontPanel* front_panel();

        void set_backlight(bool on);
        zc95_version_t hardware_version();

        void loop();

        void acc_port_reset();
        void acc_port_set_io_port_state(ExtInputPort output, bool high);

        void audio_input_enable(bool enable);
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);

        static void s_gpio_callback(uint gpio, uint32_t events);

    private:
        PCF8574* _pcf8574_main = NULL;
        PCF8574* _pcf8574_ext = NULL;
        CExtInputPortExp* _ext_input_port_exp = NULL;
        CMainBoardPortExp* _main_board_port_exp = NULL;
        CPowerManagementMk1* _mk1_pm = NULL;
        CRoutineOutput** _routine_output;
        CAnalogueCapture* _analogue_capture = NULL;
        CSavedSettings **_saved_settings = NULL;
        static HalMk1* _this;
        CFrontPanel* _front_panel = NULL;

        time_t _last_loop_time = 0;
};

#endif
