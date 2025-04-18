#include "HalMk2.h"
#include "../HwCheck/CDetermineHardwareVersion.h"
#include "../FrontPanel/CFrontPanelV02.h"

HalMk2 *HalMk2::_this = NULL;

HalMk2::HalMk2(CLedControl* led, CRoutineOutput** routine_output, CAnalogueCapture* analogue_capture, CSavedSettings** saved_settings)
{
    _this = this;
    _analogue_capture = analogue_capture;
    _saved_settings = saved_settings;
    _routine_output = routine_output;
    _tca9534_main = new TCA9534(MK2_PORT_EXP_ADDR);
    _main_board_port_exp = new CMainBoardPortExp(zc95_version_t::MKII, _tca9534_main);
    _mk2_pm = new CPowerManagementMk2(_main_board_port_exp);
    _tca9534_ext = new TCA9534(MK2_EXT_INPUT_PORT_EXP_ADDR);
    _ext_input_port_exp = new CExtInputPortExp(led, routine_output, _tca9534_ext);

    gpio_init(PIN_MK2_DISP_BL);
    gpio_set_dir(PIN_MK2_DISP_BL, GPIO_OUT);

    gpio_init(PIN_CONTROLS_INT);
    gpio_set_dir(PIN_CONTROLS_INT, GPIO_IN);

    // MKII's don't support v0.1 front panels, as these require the buttons to 
    // be plugged into a socket on the main board that no longer exists
    _front_panel = new CFrontPanelV02(saved_settings);

    // Front panel
    gpio_init(PIN_FP_INT1);
    gpio_init(PIN_FP_INT2);
    gpio_set_dir(PIN_FP_INT1, GPIO_IN);
    gpio_set_dir(PIN_FP_INT2, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_FP_INT1, GPIO_IRQ_EDGE_FALL, true, &s_gpio_callback);
    gpio_set_irq_enabled_with_callback(PIN_FP_INT2, GPIO_IRQ_EDGE_FALL, true, &s_gpio_callback);

    // External intput - triggers and acc port I/O lines
    gpio_init(PIN_EXT_INPUT_INT);
    gpio_set_dir(PIN_EXT_INPUT_INT, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_EXT_INPUT_INT, GPIO_IRQ_EDGE_FALL, true, &s_gpio_callback);
    _ext_input_port_exp->clear_input();
    _ext_input_port_exp->process(true);
}

HalMk2::~HalMk2()
{
    _this = NULL;
    delete _front_panel;
    delete _ext_input_port_exp;
    delete _tca9534_ext;
    delete _mk2_pm;
    delete _main_board_port_exp;
    delete _tca9534_main;
}

void HalMk2::loop()
{
    // On the Mk2, voltage_readings corresponds to USB VBUS voltage (but needs scaling in mk2_pm)
    if (_analogue_capture->new_voltage_readings_available())
    {
        uint8_t readings_count = 0;
        uint8_t *readings = _analogue_capture->get_voltage_readings(&readings_count);
        _mk2_pm->add_raw_adc_readings(readings, readings_count);
    }

    _mk2_pm->loop();

    if (time_us_64() - _last_loop_time > 1000000) // every second
    {
        _last_loop_time = time_us_64();
        _ext_input_port_exp->process(true);
        _main_board_port_exp->process(true);
    }
    else
    {
        _ext_input_port_exp->process(false);
        _main_board_port_exp->process(false);
        _front_panel->process(false);
    }
}

IPowerManagement* HalMk2::power_management() 
{
    return _mk2_pm;
}

CExtInputPortExp* HalMk2::external_input_port_exp()
{
    return _ext_input_port_exp;
}

CMainBoardPortExp* HalMk2::mainboard_port_exp()
{
    return _main_board_port_exp;
}

CFrontPanel* HalMk2::front_panel()
{
    return _front_panel;
}

void HalMk2::set_backlight(bool on)
{
    gpio_put(PIN_MK2_DISP_BL, on);
}

zc95_version_t HalMk2::hardware_version()
{
    return zc95_version_t::MKII;
}

void HalMk2::s_gpio_callback(uint gpio, uint32_t events)
{
    if (!_this)
        return;

    if (gpio == PIN_EXT_INPUT_INT)
        _this->external_input_port_exp()->interrupt();
    else if (gpio == PIN_FP_INT1)
        _this->_front_panel->interrupt(CFrontPanel::interrupt_t::INT1);
    else if (gpio == PIN_FP_INT2)
        _this->_front_panel->interrupt(CFrontPanel::interrupt_t::INT2);
    else if (gpio == PIN_CONTROLS_INT)
        _this->_main_board_port_exp->interrupt();
    
}
void HalMk2::acc_port_reset()
{
    _ext_input_port_exp->reset_acc_port();
}

void HalMk2::acc_port_set_io_port_state(ExtInputPort output, bool high)
{
    _ext_input_port_exp->set_acc_io_port_state(output, high);
}

void HalMk2::mic_preamp_enable(bool enable)
{
    _main_board_port_exp->mic_preamp_enable(enable);
}

void HalMk2::mic_power_enable(bool enable)
{
    _main_board_port_exp->mic_power_enable(enable);
}
