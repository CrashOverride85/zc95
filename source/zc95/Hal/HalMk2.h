#ifndef _IHALMK2_H
#define _IHALMK2_H

#include <inttypes.h>

#include "IHal.h"
#include "../CAnalogueCapture.h"
#include "../PortExpanders/TCA9534.h"
#include "PowerManagement/CPowerManagementMk2.h"

class HalMk2 : public IHal
{
    public:
        HalMk2(CLedControl* led, CRoutineOutput** routine_output, CAnalogueCapture* analogue_capture, CSavedSettings** saved_settings);
        ~HalMk2();

        IPowerManagement* power_management();
        CExtInputPortExp* external_input_port_exp();
        CMainBoardPortExp* mainboard_port_exp();
        CFrontPanel* front_panel();
        front_panel_version_t front_panel_version();

        void set_backlight(bool on);
        zc95_version_t hardware_version();

        void loop();
        void acc_port_reset();
        void acc_port_set_io_port_state(ExtInputPort output, bool high);

        void audio_input_enable(bool enable) {};
        void mic_preamp_enable(bool enable);
        void mic_power_enable(bool enable);

        static void s_gpio_callback(uint gpio, uint32_t events);

    private:
        TCA9534* _tca9534_ext = NULL;
        TCA9534* _tca9534_main = NULL;
        CExtInputPortExp* _ext_input_port_exp = NULL;
        CMainBoardPortExp* _main_board_port_exp = NULL;
        CPowerManagementMk2* _mk2_pm = NULL;
        CRoutineOutput** _routine_output;
        CAnalogueCapture* _analogue_capture = NULL;
        CSavedSettings** _saved_settings;
        static HalMk2* _this;
        CFrontPanel* _front_panel = NULL;
        time_t _last_loop_time = 0;
        front_panel_version_t _front_panel_version = front_panel_version_t::v0_2; // MKII's can't work with v0.1 FPs
};

#endif
