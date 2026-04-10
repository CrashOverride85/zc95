#include "CDetermineHardwareVersion.h"
#include "hardware/i2c.h"
#include "../CUtil.h"
#include "../../common/zc95_config.h"

zc95_version_t CDetermineHardwareVersion::get_hardware_version()
{
    uint8_t rx_data = 0;

    // The MK1 uses PCF8574 expanders, whilst on the MKII it's TCA9534's. The addresses are the same on MKI & MKII.
    // Try writing a value to the TCA9534's config register, then reading it back. If that works, assume we're
    // running on a MKII. If the device is found but the read values don't match, assume a PCF8574 so MKI.

    rx_data = 0;
    i2c_write_timeout_us(i2c0, MK1_EXT_INPUT_PORT_EXP_ADDR, &rx_data, 1, false, 1000);

    uint8_t buffer[2] = {0};

    buffer[0] = 3; // config reg
    buffer[1] = ~(1 << 3); // P3 isn't connected

    // Write 0xAA to config register
    int bytes_written = i2c_write(__func__, MK1_EXT_INPUT_PORT_EXP_ADDR, buffer, 2, false);
    if (bytes_written != 2)
    {
        printf("CDetermineHardwareVersion::get_hardware_version: write failed! i2c bytes_written = %d (1)\n", bytes_written);
        return zc95_version_t::UNKNOWN;
    }

    // Read value of config register
    buffer[0] = 3;
    bytes_written = i2c_write(__func__, MK1_EXT_INPUT_PORT_EXP_ADDR, buffer, 1, false);
    if (bytes_written != 1)
    {
        printf("CDetermineHardwareVersion::get_hardware_version: write failed! i2c bytes_written = %d (2)\n", bytes_written);
        return zc95_version_t::UNKNOWN;
    }

    int retval = i2c_read(__func__, MK1_EXT_INPUT_PORT_EXP_ADDR, buffer, 1, false);
    if (retval == -1)
    {
        printf("CDetermineHardwareVersion::get_hardware_version: read failed! i2c bytes_written = %d\n", bytes_written);
        return zc95_version_t::UNKNOWN;
    }

    if (buffer[0] == buffer[1])
    {
        // restore config register to power on default
        buffer[0] = 3; // config reg
        buffer[1] = 0xFF;
        i2c_write(__func__, MK1_EXT_INPUT_PORT_EXP_ADDR, buffer, 2, false);

        return zc95_version_t::MKII;
    }
    else
    {
        buffer[0] = 0xFF; // restore power on default if MK1
        bytes_written = i2c_write(__func__, MK1_EXT_INPUT_PORT_EXP_ADDR, buffer, 1, false);
        return zc95_version_t::MKI;
    }
}

// Try to determine the front panel version.
// Identify v0.1 by the presence of the I/O expander on 0x26 (on v0.2 the I/O expander is on 0x38, but needs to be written to before it'll respond)
// Identify v0.2 by the presence of the ADC 0x49
front_panel_version_t CDetermineHardwareVersion::get_front_panel_version()
{
    uint8_t rx_data = 0;

    bool v0_1 = (i2c_read_timeout_us(i2c0, FP_0_1_PORT_EXP_ADDR, &rx_data, 1, false, 1000) > 0);
    bool v0_2 = (i2c_read_timeout_us(i2c0, FP_0_2_ADC_ADDR, &rx_data, 1, false, 1000) > 0);

    if (v0_1 && v0_2)
    {
        printf("ERROR: unable to determine front panel version: device found at address 0x%x (v0.1 IO expander) AND address 0x%x (v0.2 ADC) ?!\n", 
            FP_0_1_PORT_EXP_ADDR, FP_0_2_ADC_ADDR);
        printf("       (only one - either - expected)\n");
        return front_panel_version_t::UNKNOWN;
    }
    else if (v0_1)
    {
        printf("Front panel version: 0.1\n");
        return front_panel_version_t::v0_1;
    }
    else if (v0_2)
    {
        printf("Front panel version: 0.2+\n");
        return front_panel_version_t::v0_2;
    }
    else
    {
        printf("ERROR: unable to determine front panel version: No device found on either address 0x%x (v0.1 IO expander) or address 0x%x (v0.2 ADC)\n",
             FP_0_1_PORT_EXP_ADDR, FP_0_2_ADC_ADDR);
        return front_panel_version_t::UNKNOWN;
    }
}
