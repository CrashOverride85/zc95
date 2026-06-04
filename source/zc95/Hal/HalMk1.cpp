#include "HalMk1.h"
#include "../HwCheck/CDetermineHardwareVersion.h"
#include "../FrontPanel/CFrontPanelV01.h"
#include "../FrontPanel/CFrontPanelV02.h"

HalMk1 *HalMk1::_this = NULL;

HalMk1::HalMk1(CLedControl* led, CRoutineOutput** routine_output, CAnalogueCapture* analogue_capture, CSavedSettings** saved_settings)
{
    _this = this;
    _analogue_capture = analogue_capture;
    _saved_settings = saved_settings;
    _routine_output = routine_output;
    _pcf8574_main = new PCF8574(MK1_CONTROLS_PORT_EXP_ADDR);
    _main_board_port_exp = new CMainBoardPortExp(zc95_version_t::MKI, hw_variant_t::NA, _pcf8574_main);
    _mk1_pm = new CPowerManagementMk1();
    _pcf8574_ext = new PCF8574(MK1_EXT_INPUT_PORT_EXP_ADDR);
    _ext_input_port_exp = new CExtInputPortExp(led, routine_output, _pcf8574_ext);
    _led = led;

    for (uint x=0; x < 10; x++)
        _mk1_pm->get_battery_readings();

    gpio_init(PIN_MK1_DISP_RST);
    gpio_set_dir(PIN_MK1_DISP_RST, GPIO_OUT);
    gpio_put(PIN_MK1_DISP_RST, true); // RST is active low

    gpio_init(PIN_CONTROLS_INT);
    gpio_set_dir(PIN_CONTROLS_INT, GPIO_IN);

    _front_panel_version = CDetermineHardwareVersion::get_front_panel_version();
    if (_front_panel_version == front_panel_version_t::v0_2)
        _front_panel = new CFrontPanelV02(saved_settings);
    else
        _front_panel = new CFrontPanelV01(_main_board_port_exp);

    // Front panel
    gpio_init(PIN_FP_INT1);
    gpio_init(PIN_FP_INT2);
    gpio_set_dir(PIN_FP_INT1, GPIO_IN);
    gpio_set_dir(PIN_FP_INT2, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_FP_INT1, GPIO_IRQ_EDGE_FALL, true, &s_gpio_callback);
    gpio_set_irq_enabled_with_callback(PIN_FP_INT2, GPIO_IRQ_EDGE_FALL, true, &s_gpio_callback);

    if (_front_panel_version == front_panel_version_t::v0_1)
        gpio_set_irq_enabled_with_callback(PIN_CONTROLS_INT, GPIO_IRQ_EDGE_FALL, true, &s_gpio_callback);

    // External intput - triggers and acc port I/O lines
    gpio_init(PIN_EXT_INPUT_INT);
    gpio_set_dir(PIN_EXT_INPUT_INT, GPIO_IN);
    gpio_set_irq_enabled_with_callback(PIN_EXT_INPUT_INT, GPIO_IRQ_EDGE_FALL, true, &s_gpio_callback);
    _ext_input_port_exp->clear_input();
    _ext_input_port_exp->process(true);
}

HalMk1::~HalMk1()
{
    _this = NULL;
    delete _front_panel;
    delete _ext_input_port_exp;
    delete _pcf8574_ext;
    delete _mk1_pm;
    delete _main_board_port_exp;
    delete _pcf8574_main;
}

void HalMk1::loop()
{
    // On the Mk1, voltage_readings corresponds to battery voltage (but needs scaling in mk1_pm)
    if (_analogue_capture->new_voltage_readings_available())
    {
        uint8_t readings_count = 0;
        uint8_t *readings = _analogue_capture->get_voltage_readings(&readings_count);
        _mk1_pm->add_raw_adc_readings(readings, readings_count);
    }

    _mk1_pm->loop();

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

IPowerManagement* HalMk1::power_management() 
{
    return _mk1_pm;
}

CExtInputPortExp* HalMk1::external_input_port_exp()
{
    return _ext_input_port_exp;
}

CMainBoardPortExp* HalMk1::mainboard_port_exp()
{
    return _main_board_port_exp;
}

CFrontPanel* HalMk1::front_panel()
{
    return _front_panel;
}

void HalMk1::set_backlight(bool on)
{
    _main_board_port_exp->set_lcd_backlight(on);
}

zc95_version_t HalMk1::hardware_version()
{
    return zc95_version_t::MKI;
}

CLedControl* HalMk1::led_control()
{
    return _led;
}

void HalMk1::s_gpio_callback(uint gpio, uint32_t events)
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

void HalMk1::acc_port_reset()
{
    _ext_input_port_exp->reset_acc_port();
}

void HalMk1::acc_port_set_io_port_state(ExtInputPort output, ExtInputPortState state)
{
    _ext_input_port_exp->set_acc_io_port_state(output, state);
}

void HalMk1::audio_input_enable(bool enable)
{
    _main_board_port_exp->audio_input_enable(enable);
}

void HalMk1::mic_preamp_enable(bool enable)
{
    _main_board_port_exp->mic_preamp_enable(enable);
}

void HalMk1::mic_power_enable(bool enable)
{
    _main_board_port_exp->mic_power_enable(enable);
}

front_panel_version_t HalMk1::front_panel_version()
{
    return _front_panel_version;
}
