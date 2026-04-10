
#ifndef I2C_ENUMS_H
#define I2C_ENUMS_H

#ifdef __cplusplus
extern "C" {
#endif

enum Zc624Register {
    ZC624_REG_TYPELOW             = 0x00,
    ZC624_REG_TYPEHIGH            = 0x01,
    ZC624_REG_VERSIONMAJOR        = 0x02,
    ZC624_REG_VERSIONMINOR        = 0x03,
    ZC624_REG_OVERALLSTATUS       = 0x0F,
    ZC624_REG_CHAN0STATUS         = 0x10,
    ZC624_REG_CHAN1STATUS         = 0x11,
    ZC624_REG_CHAN2STATUS         = 0x12,
    ZC624_REG_CHAN3STATUS         = 0x13,
    ZC624_REG_VERSTRSTART         = 0x20,   // main firmware version
    ZC624_REG_VERSTREND           = 0x34,
    ZC624_REG_TESTVAL             = 0x40,
    ZC624_REG_DATABLOCKREAD       = 0x41,
    ZC624_REG_FW_BLOCK_COUNT      = 0x42,
    ZC624_REG_BL_VERSTRSTART      = 0x43,   // bootloader version
    ZC624_REG_BL_VERSTREND        = 0x57,
    ZC624_REG_CHANNELISOLATION    = 0x80,
    ZC624_REG_BOOTLOADER          = 0x81,
    ZC624_REG_DATABLOCKWRITE      = 0x82,
    ZC624_REG_ERASEFIRMWARE       = 0x83
};

enum Zc624RegisterEraseFirmware
{
    ZC624_REG_ERASEFW_NA       = 0x00, // Firmware cannot be erased - i.e. zc624 has exited the bootloader
    ZC624_REG_ERASEFW_ERASABLE = 0x01, // Firmware can be (or has been) erased
    ZC624_REG_ERASEFW_ERASE    = 0x02  // Write to start erase
};

enum Zc624BootloadState
{
    ZC624_REG_BOOTLOADER_STATE_IN_BOOTLOADER     = 0x00,
    ZC624_REG_BOOTLOADER_STATE_RUN_MAIN_FIRMWARE = 0x01,
    ZC624_REG_BOOTLOADER_STATE_INIT              = 0x02
};

enum ZC624RegOverallStatus
{
    ZC624_OVERALL_STATUS_STARTUP       = 0x00,
    ZC624_OVERALL_STATUS_READY         = 0x01,
    ZC624_OVERALL_STATUS_FAULT         = 0x02,
    ZC624_OVERALL_STATUS_FAULTFIRMWARE = 0x03
};

#ifdef __cplusplus
}
#endif

#endif
