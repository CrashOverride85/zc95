/**
 *     BQ27441 LiPo Fuel Gauge
 *
 *     Copyright (c) 2020 Vitaliy Nimych (Cvetaev) @ cvetaevvitaliy@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "bq27441.h"
#include "pico/time.h"
#include <stddef.h>



/*****************************************************************************
 ************************ I2C Read and Write Routines ************************
 *****************************************************************************/
static BQ27441_ctx_t ctx = {0};

/**
  * @brief  Read generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to read
  * @param  len   data len
  * @param  data  pointer to buffer that store the data read(ptr)
  * @retval       interface status (MANDATORY: return 0 -> no Error)
  *
  */
int16_t BQ27441_i2cReadBytes(uint8_t reg, uint8_t *data, uint8_t len)
{
    int16_t ret;
    ret = ctx.read_reg(ctx.BQ27441_i2c_address, reg, data, len);
    return ret;
}


/**
  * @brief  Write generic device register
  *
  * @param  ctx   read / write interface definitions(ptr)
  * @param  reg   register to write
  * @param  data  pointer to data to write in register reg(ptr)
  * @param  len   data len
  * @retval       interface status (MANDATORY: return 0 -> no Error)
  *
  */
int16_t BQ27441_i2cWriteBytes(uint8_t reg, uint8_t *data, uint8_t len)
{
    int16_t ret;
    ret = ctx.write_reg(ctx.BQ27441_i2c_address, reg, data, len);
    return ret;
}



static bool BQ27441_sealed (void);
static bool BQ27441_seal (void);
static bool BQ27441_unseal (void);
static uint16_t BQ27441_opConfig (void);
static bool BQ27441_writeOpConfig (uint16_t value);
bool BQ27441_softReset (void);
static uint16_t BQ27441_readWord (uint16_t subAddress);
static uint16_t BQ27441_readControlWord (uint16_t function);
static bool BQ27441_executeControlWord (uint16_t function);

/*****************************************************************************
 ************************** Extended Data Commands ***************************
 *****************************************************************************/

static bool BQ27441_blockDataControl (void);
static bool BQ27441_blockDataClass (uint8_t id);
static bool BQ27441_blockDataOffset (uint8_t offset);
static uint8_t BQ27441_blockDataChecksum (void);
static uint8_t BQ27441_readBlockData (uint8_t offset);
static bool BQ27441_writeBlockData (uint8_t offset, uint8_t data);
static uint8_t BQ27441_computeBlockChecksum (void);
static bool BQ27441_writeBlockChecksum (uint8_t csum);
static uint8_t BQ27441_readExtendedData (uint8_t classID, uint8_t offset);
static bool BQ27441_writeExtendedData (uint8_t classID, uint8_t offset, uint8_t *data, uint8_t len);


static volatile bool sealFlag = false; // Global to identify that IC was previously sealed
static volatile bool userConfigControl = false; // Global to identify that user has control over

static uint8_t constrain(const uint8_t x, const uint8_t a, const uint8_t b);


/**
 * Initializes I2C and verifies communication with the BQ27441.
 * Must be called before using any other functions.
 *
 * @return true if communication was successful.
 * */
bool BQ27441_init(BQ27441_ctx_t *dev) {


    if (dev == NULL)
        return false;

    ctx.read_reg = dev->read_reg;
    ctx.write_reg = dev->write_reg;
    ctx.BQ27441_i2c_address = dev->BQ27441_i2c_address;

    // Read deviceType from BQ27441
    if (BQ27441_deviceType() == BQ27441_DEVICE_ID) {
        return true; // If device ID is valid, return true
    } else
        return false;

}

/**
 * Configures the design capacity of the connected battery.
 *
 * @param capacity of battery (unsigned 16-bit value)
 * @return true if capacity successfully set.
 * */
bool BQ27441_setCapacity(uint16_t capacity) {
    // Write to STATE subclass (82) of BQ27441 extended memory.
    // Offset 0x0A (10)
    // Design capacity is a 2-byte piece of data - MSB first
    // Unit: mAh
    uint8_t capacityData[2] = {capacity >> 8, capacity & 0x00FF};
    return BQ27441_writeExtendedData(BQ27441_ID_STATE, 10, capacityData, 2);
}


/**
 * Configures the Hibernate I the gauge enters HIBERNATE mode.
 *
 * @param mA (unsigned 16-bit value)
 * @return true if capacity successfully set.
 * */
bool BQ27441_setHibernateCurrent(uint16_t current_mA) {
    // Write to STATE subclass (68) of BQ27441 extended memory.
    // Offset 0x07 (7)
    // Hibernate I is a 2-byte piece of data - MSB first
    // Unit: mA
    uint8_t current_mA_Data[2] = {current_mA >> 8, current_mA & 0x00FF};
    return BQ27441_writeExtendedData(BQ27441_ID_POWER, 7, current_mA_Data, 2);
}

/**
 * Configures the design energy of the connected battery.
 *
 * @param energy of battery (unsigned 16-bit value)
 * @return true if energy successfully set.
*/
bool BQ27441_setDesignEnergy(uint16_t energy) {
    // Write to STATE subclass (82) of BQ27441 extended memory.
    // Offset 0x0C (12)
    // Design energy is a 2-byte piece of data - MSB first
    // Unit: mWh
    uint8_t energyData[2] = {energy >> 8, energy & 0x00FF};
    return BQ27441_writeExtendedData(BQ27441_ID_STATE, 12, energyData, 2);
}

/**
 * Configures terminate voltage (lowest operational voltage of battery powered circuit)
 *
 * @param voltage of battery (unsigned 16-bit value)
 * @return true if energy successfully set.
 * */
bool BQ27441_setTerminateVoltageMin(uint16_t voltage) {
    // Write to STATE subclass (82) of BQ27441 extended memory.
    // Offset 0x0F (16)
    // Termiante voltage is a 2-byte piece of data - MSB first
    // Unit: mV
    // Min 2500, Max 3700
    if (voltage < 2500) voltage = 2500;
    if (voltage > 3700) voltage = 3700;

    uint8_t tvData[2] = {voltage >> 8, voltage & 0x00FF};
    return BQ27441_writeExtendedData(BQ27441_ID_STATE, 16, tvData, 2);
}

bool BQ27441_setChargeVChgTermination(uint16_t voltage){
    // V at Chg Term
    // Write to STATE subclass (82) of BQ27441 extended memory.
    // Offset 0x21 (33)
    // Termiante voltage is a 2-byte piece of data - MSB first
    // Unit: mV
    // Min 0, Max 5000

    uint8_t tvData[2] = {voltage >> 8, voltage & 0x00FF};
    return BQ27441_writeExtendedData(BQ27441_ID_STATE, 33, tvData, 2);

}

// Configures taper rate of connected battery.
bool BQ27441_setTaperRateTime(uint16_t rate) {
    // Write to STATE subclass (82) of BQ27441 extended memory.
    // Offset 0x1B (27)
    // Termiante voltage is a 2-byte piece of data - MSB first
    // Unit: 0.1 Hr
    // Max 2000
    // default 100
    if (rate > 2000)
        rate = 2000;

    uint8_t trData[2] = {rate >> 8, rate & 0x00F};
    return BQ27441_writeExtendedData(BQ27441_ID_STATE, 27, trData, 2);
}

bool BQ27441_setTaperRateVoltage(uint16_t voltage) {
    // Write to STATE subclass (82) of BQ27441 extended memory.
    // Offset 0x1B (27)
    // Termiante voltage is a 2-byte piece of data - MSB first
    // Unit: mV
    // Max 2000
    // default 4100
    if (voltage > 5000)
        voltage = 5000;

    uint8_t trData[2] = {voltage >> 8, voltage & 0x00FF};
    return BQ27441_writeExtendedData(BQ27441_ID_STATE, 29, trData, 2);
}

/*****************************************************************************
 ********************** Battery Characteristics Functions ********************
 *****************************************************************************/

/**
 * Reads and returns the battery voltage
 *
 * @return battery voltage in mV
 * */
uint16_t BQ27441_voltage(void) {
    return BQ27441_readWord(BQ27441_COMMAND_VOLTAGE);
}

/**
 * Reads and returns the specified current measurement
 * @param current_measure enum specifying current value to be read
 * @return specified current measurement in mA. >0 indicates charging.
 * */
int16_t BQ27441_current(current_measure type) {
    //type = AVG;
    int16_t current = 0;
    switch (type) {
        case AVG:
            current = (int16_t) BQ27441_readWord(BQ27441_COMMAND_AVG_CURRENT);
            break;
        case STBY:
            current = (int16_t) BQ27441_readWord(BQ27441_COMMAND_STDBY_CURRENT);
            break;
        case MAX:
            current = (int16_t) BQ27441_readWord(BQ27441_COMMAND_MAX_CURRENT);
            break;
    }

    return current;
}

/**
 * Reads and returns the specified capacity measurement
 *
 * @param capacity_measure enum specifying capacity value to be read
 * @return specified capacity measurement in mAh.
 * */
uint16_t BQ27441_capacity(capacity_measure type) {
    //type = REMAIN;
    uint16_t capacity = 0;
    switch (type) {
        case REMAIN:
            return BQ27441_readWord(BQ27441_COMMAND_REM_CAPACITY);
            break;
        case FULL:
            return BQ27441_readWord(BQ27441_COMMAND_FULL_CAPACITY);
            break;
        case AVAIL:
            capacity = BQ27441_readWord(BQ27441_COMMAND_NOM_CAPACITY);
            break;
        case AVAIL_FULL:
            capacity = BQ27441_readWord(BQ27441_COMMAND_AVAIL_CAPACITY);
            break;
        case REMAIN_F:
            capacity = BQ27441_readWord(BQ27441_COMMAND_REM_CAP_FIL);
            break;
        case REMAIN_UF:
            capacity = BQ27441_readWord(BQ27441_COMMAND_REM_CAP_UNFL);
            break;
        case FULL_F:
            capacity = BQ27441_readWord(BQ27441_COMMAND_FULL_CAP_FIL);
            break;
        case FULL_UF:
            capacity = BQ27441_readWord(BQ27441_COMMAND_FULL_CAP_UNFL);
            break;
        case TRUE_REMAIN:
            capacity = BQ27441_readWord(BQ27441_COMMAND_TRUE_REM_CAP);
            break;
        case DESIGN:
            capacity = BQ27441_readWord(BQ27441_EXTENDED_CAPACITY);
            break;

        default:
            return BQ27441_readWord(BQ27441_COMMAND_REM_CAPACITY);
    }

    return capacity;
}

/**
 * Reads and returns measured average power
 *
 * @return average power in mAh. >0 indicates charging.
 * */
int16_t BQ27441_power(void) {
    return (int16_t) BQ27441_readWord(BQ27441_COMMAND_AVG_POWER);
}

/**
 * Reads and returns specified state of charge measurement
 *
 * @param soc_measure enum specifying filtered or unfiltered measurement
 * @return specified state of charge measurement in %
 * */
uint16_t BQ27441_soc(soc_measure type) {
    //type = FILTERED;
    uint16_t socRet = 0;
    switch (type) {
        case FILTERED:
            socRet = BQ27441_readWord(BQ27441_COMMAND_SOC);
            break;
        case UNFILTERED:
            socRet = BQ27441_readWord(BQ27441_COMMAND_SOC_UNFL);
            break;
    }

    return socRet;
}

/**
 * Reads and returns specified state of health measurement
 *
 * @param soh_measure enum specifying filtered or unfiltered measurement
 * @return specified state of health measurement in %, or status bits
 * */
uint8_t BQ27441_soh(soh_measure type) {
    //type = PERCENT;
    uint16_t sohRaw = BQ27441_readWord(BQ27441_COMMAND_SOH);
    uint8_t sohStatus = sohRaw >> 8;
    uint8_t sohPercent = sohRaw & 0x00FF;

    if (type == PERCENT)
        return sohPercent;
    else
        return sohStatus;
}

/**
 * Reads and returns specified temperature measurement
 *
 * @param temp_measure enum specifying internal or battery measurement
 * @return specified temperature measurement in degrees C
 * */
uint16_t BQ27441_temperature(temp_measure type) {
    //type = BATTERY;
    uint16_t temp = 0;
    switch (type) {
        case BATTERY:
            temp = BQ27441_readWord(BQ27441_COMMAND_TEMP);
            break;
        case INTERNAL_TEMP:
            temp = BQ27441_readWord(BQ27441_COMMAND_INT_TEMP);
            break;
    }
    return temp;
}

/*****************************************************************************
 ************************** GPOUT Control Functions **************************
 *****************************************************************************/

/**
 * Get GPOUT polarity setting (active-high or active-low)
 *
 * @return true if active-high, false if active-low
 * */
bool BQ27441_GPOUTPolarity(void) {
    uint16_t opConfigRegister = BQ27441_opConfig();

    return (opConfigRegister & BQ27441_OPCONFIG_GPIOPOL);
}

bool BQ27441_setSLEEPenable(bool enable) {
    uint16_t oldOpConfig = BQ27441_opConfig();

    // Check to see if we need to update opConfig:
    if ((enable && (oldOpConfig & BQ27441_OPCONFIG_GPIOPOL)) ||
        (!enable && !(oldOpConfig & BQ27441_OPCONFIG_GPIOPOL)))
        return true;

    uint16_t newOpConfig = oldOpConfig;
    if (enable)
        newOpConfig |= BQ27441_OPCONFIG_SLEEP;
    else
        newOpConfig &= ~(BQ27441_OPCONFIG_SLEEP);

    return BQ27441_writeOpConfig(newOpConfig);
}



/**
 * Set GPOUT polarity to active-high or active-low
 *
 * @param activeHigh is true if active-high, false if active-low
 * @return true on success
 * */
bool BQ27441_setGPOUTPolarity(bool activeHigh) {
    uint16_t oldOpConfig = BQ27441_opConfig();

    // Check to see if we need to update opConfig:
    if ((activeHigh && (oldOpConfig & BQ27441_OPCONFIG_GPIOPOL)) ||
        (!activeHigh && !(oldOpConfig & BQ27441_OPCONFIG_GPIOPOL)))
        return true;

    uint16_t newOpConfig = oldOpConfig;
    if (activeHigh)
        newOpConfig |= BQ27441_OPCONFIG_GPIOPOL;
    else
        newOpConfig &= ~(BQ27441_OPCONFIG_GPIOPOL);

    return BQ27441_writeOpConfig(newOpConfig);
}



/**
 * Get GPOUT function (BAT_LOW or SOC_INT)
 *
 * @return true if BAT_LOW or false if SOC_INT
 * */
bool BQ27441_GPOUTFunction(void) {
    uint16_t opConfigRegister = BQ27441_opConfig();

    return (opConfigRegister & BQ27441_OPCONFIG_BATLOWEN);
}

/**
 * Set GPOUT function to BAT_LOW or SOC_INT
 *
 * @param function should be either BAT_LOW or SOC_INT
 * @return true on success
 * */
bool BQ27441_setGPOUTFunction(gpout_function function) {
    uint16_t oldOpConfig = BQ27441_opConfig();

    // Check to see if we need to update opConfig:
    if ((function && (oldOpConfig & BQ27441_OPCONFIG_BATLOWEN)) ||
        (!function && !(oldOpConfig & BQ27441_OPCONFIG_BATLOWEN)))
        return true;

    // Modify BATLOWN_EN bit of opConfig:
    uint16_t newOpConfig = oldOpConfig;
    if (function)
        newOpConfig |= BQ27441_OPCONFIG_BATLOWEN;
    else
        newOpConfig &= ~(BQ27441_OPCONFIG_BATLOWEN);

    // Write new opConfig
    return BQ27441_writeOpConfig(newOpConfig);
}


bool BQ27441_set_BI_PU_EN(bool detect_bat_pin_enable) {
    uint16_t oldOpConfig = BQ27441_opConfig();

    // Check to see if we need to update opConfig:
    if ((detect_bat_pin_enable && (oldOpConfig & BQ27441_OPCONFIG_GPIOPOL)) ||
        (!detect_bat_pin_enable && !(oldOpConfig & BQ27441_OPCONFIG_GPIOPOL)))
        return true;

    uint16_t newOpConfig = oldOpConfig;
    if (detect_bat_pin_enable)
        newOpConfig |= BQ27441_OPCONFIG_BI_PU_EN;
    else
        newOpConfig &= ~(BQ27441_OPCONFIG_BI_PU_EN);

    return BQ27441_writeOpConfig(newOpConfig);

}
/**
 * Get SOC1_Set Threshold - threshold to set the alert flag
 *
 * @return state of charge value between 0 and 100%
 * */
uint8_t BQ27441_SOC1SetThreshold(void)
{
    return BQ27441_readExtendedData(BQ27441_ID_DISCHARGE, 0);
}

/**
 * Get SOC1_Clear Threshold - threshold to clear the alert flag
 *
 * @return state of charge value between 0 and 100%
 * */
uint8_t BQ27441_SOC1ClearThreshold(void)
{
    return BQ27441_readExtendedData(BQ27441_ID_DISCHARGE, 1);
}

/**
 * Set the SOC1 set and clear thresholds to a percentage
 *
 * @param set and clear percentages between 0 and 100. clear > set.
 * @return true on success
 * */
bool BQ27441_setSOC1Thresholds(uint8_t set, uint8_t clear)
{
    uint8_t thresholds[2];
    thresholds[0] = constrain(set, 0, 100);
    thresholds[1] = constrain(clear, 0, 100);
    return BQ27441_writeExtendedData(BQ27441_ID_DISCHARGE, 0, thresholds, 2);
}

/**
 * Get SOCF_Set Threshold - threshold to set the alert flag
 *
 * @return state of charge value between 0 and 100%
 * */
uint8_t BQ27441_SOCFSetThreshold(void)
{
    return BQ27441_readExtendedData(BQ27441_ID_DISCHARGE, 2);
}

/**
 * Get SOCF_Clear Threshold - threshold to clear the alert flag
 *
 * @return state of charge value between 0 and 100%
 * */
uint8_t BQ27441_SOCFClearThreshold(void)
{
    return BQ27441_readExtendedData(BQ27441_ID_DISCHARGE, 3);
}

/**
 * Set the SOCF set and clear thresholds to a percentage
 *
 * @param set and clear percentages between 0 and 100. clear > set.
 * @return true on success
 * */
bool BQ27441_setSOCFThresholds(uint8_t set, uint8_t clear)
{
    uint8_t thresholds[2];
    thresholds[0] = constrain(set, 0, 100);
    thresholds[1] = constrain(clear, 0, 100);
    return BQ27441_writeExtendedData(BQ27441_ID_DISCHARGE, 2, thresholds, 2);
}

/**
 * Check if the SOC1 flag is set in flags()
 *
 * @return true if flag is set
 * */
bool BQ27441_socFlag(void)
{
    uint16_t flagState = BQ27441_flags();

    return flagState & BQ27441_FLAG_SOC1;
}

/**
 * Check if the SOCF flag is set in flags()
 *
 * @return true if flag is set
 * */
bool BQ27441_socfFlag(void)
{
    uint16_t flagState = BQ27441_flags();

    return flagState & BQ27441_FLAG_SOCF;

}

/**
 * Check if the ITPOR flag is set in flags()
 *
 * @return true if flag is set
 * */
bool BQ27441_itporFlag(void)
{
    uint16_t flagState = BQ27441_flags();

    return flagState & BQ27441_FLAG_ITPOR;
}

bool BQ27441_initComp(void){

    uint16_t stat = BQ27441_status();
    return stat & BQ27441_STATUS_INITCOMP;

}

/**
 * Check if the FC flag is set in flags()
 *
 * @return true if flag is set
 * */
bool BQ27441_fcFlag(void)
{
    uint16_t flagState = BQ27441_flags();

    return flagState & BQ27441_FLAG_FC;
}

/**
 * Check if the CHG flag is set in flags()
 *
 * @return true if flag is set
 * */
bool BQ27441_chgFlag(void)
{
    uint16_t flagState = BQ27441_flags();

    return flagState & BQ27441_FLAG_CHG;
}

/**
 * Check if the DSG flag is set in flags()
 *
 * @return true if flag is set
 * */
bool BQ27441_dsgFlag(void)
{
    uint16_t flagState = BQ27441_flags();

    return flagState & BQ27441_FLAG_DSG;
}

/**
 * Get the SOC_INT interval delta
 *
 * @return interval percentage value between 1 and 100
 * */
uint8_t BQ27441_sociDelta(void)
{
    return BQ27441_readExtendedData(BQ27441_ID_STATE, 26);
}

/**
 * Set the SOC_INT interval delta to a value between 1 and 100
 *
 * @param interval percentage value between 1 and 100
 * @return true on success
 * */
bool BQ27441_setSOCIDelta(uint8_t delta)
{
    uint8_t soci = constrain(delta, 0, 100);
    return BQ27441_writeExtendedData(BQ27441_ID_STATE, 26, &soci, 1);
}

/**
 * Pulse the GPOUT pin - must be in SOC_INT mode
 *
 * @return true on success
 * */
bool BQ27441_pulseGPOUT(void)
{
    return BQ27441_executeControlWord(BQ27441_CONTROL_PULSE_SOC_INT);
}

bool BQ27441_SET_HIBERNATE(void)
{
    return BQ27441_executeControlWord(BQ27441_CONTROL_SET_HIBERNATE);
}

bool BQ27441_CLEAR_HIBERNATE(void)
{
    return BQ27441_executeControlWord(BQ27441_CONTROL_CLEAR_HIBERNATE);
}

/*****************************************************************************
 *************************** Control Sub-Commands ****************************
 *****************************************************************************/

/**
 * Read the device type - should be 0x0421
 *
 * @return 16-bit value read from DEVICE_TYPE subcommand
 * */
uint16_t BQ27441_deviceType(void)
{
    return BQ27441_readControlWord(BQ27441_CONTROL_DEVICE_TYPE);
}

/**
 * Enter configuration mode - set userControl if calling from an App
 * and you want control over when to exitConfig.
 *
 * @param userControl is true if the App is handling entering
 * and exiting config mode (should be false in library calls).
 * @return true on success
 * */
bool BQ27441_enterConfig(bool userControl) {

    if (userControl)
    {
        userConfigControl = true;

        if (BQ27441_sealed())
        {
            sealFlag = true;
            BQ27441_unseal(); // Must be unsealed before making changes
        }

        if (BQ27441_executeControlWord(BQ27441_CONTROL_SET_CFGUPDATE))
        {
            int16_t timeout = BQ72441_I2C_TIMEOUT;
            while ((timeout--) && (!(BQ27441_flags() & BQ27441_FLAG_CFGUPMODE)))
                sleep_ms(10);

            if (timeout > 0)
                return true;
            else
                return false;
        }
        else
            return false;
    }
    else
        return false;

}

/**
 * Exit configuration mode with the option to perform a resimulation
 *
 * @param resim is true if resimulation should be performed after exiting
 * @return true on success
 * */
bool BQ27441_exitConfig(bool resim) {

    //resim = true;
    // There are two methods for exiting config mode:
    //    1. Execute the EXIT_CFGUPDATE command
    //    2. Execute the SOFT_RESET command
    // EXIT_CFGUPDATE exits config mode _without_ an OCV (open-circuit voltage)
    // measurement, and without resimulating to update unfiltered-SoC and SoC.
    // If a new OCV measurement or resimulation is desired, SOFT_RESET or
    // EXIT_RESIM should be used to exit config mode.
    if (resim) {
        if (BQ27441_softReset()) {
            int16_t timeout = BQ72441_I2C_TIMEOUT;
            while ((timeout--) && ((BQ27441_flags() & BQ27441_FLAG_CFGUPMODE)))
                sleep_ms(1);
            if (timeout > 0) {
                if (sealFlag) BQ27441_seal(); // Seal back up if we IC was sealed coming in
                return true;
            }
        }
        return false;
    } else {
        return BQ27441_executeControlWord(BQ27441_CONTROL_EXIT_CFGUPDATE);
    }
}

/**
 * Read the flags() command
 *
 * @return 16-bit representation of flags() command register
 * */
uint16_t BQ27441_flags(void) {
    return BQ27441_readWord(BQ27441_COMMAND_FLAGS);
}

/**
 * Read the CONTROL_STATUS subcommand of control()
 *
 * @return 16-bit representation of CONTROL_STATUS subcommand
 * */
uint16_t BQ27441_status(void) {
    return BQ27441_readControlWord(BQ27441_CONTROL_STATUS);
}

/***************************** Private Functions *****************************/

/**
 * Check if the BQ27441-G1A is sealed or not.
 *
 * @return true if the chip is sealed
 * */
static bool BQ27441_sealed(void) {
    uint16_t stat = BQ27441_status();
    return stat & BQ27441_STATUS_SS;
}

/**
 * Seal the BQ27441-G1A
 *
 * @return true on success
 * */
static bool BQ27441_seal(void) {
    return BQ27441_readControlWord(BQ27441_CONTROL_SEALED);
}

/**
 * UNseal the BQ27441-G1A
 *
 * @return true on success
 * */
static bool BQ27441_unseal(void) {
    // To unseal the BQ27441, write the key to the control
    // command. Then immediately write the same key to control again.
    if (BQ27441_readControlWord(BQ27441_UNSEAL_KEY)) {
        return BQ27441_readControlWord(BQ27441_UNSEAL_KEY);
    }
    return false;
}

/**
 * Read the 16-bit opConfig register from extended data
 *
 * @return opConfig register contents
 * */
static uint16_t BQ27441_opConfig(void) {
    return BQ27441_readWord(BQ27441_EXTENDED_OPCONFIG);
}

/**
 * Write the 16-bit opConfig register in extended data
 *
 * @param New 16-bit value for opConfig
 * @return true on success
 * */
static bool BQ27441_writeOpConfig(uint16_t value) {

    uint8_t opConfigData[2] = {value >> 8, value & 0x00FF};

    // OpConfig register location: BQ27441_ID_REGISTERS id, offset 0
    return BQ27441_writeExtendedData(BQ27441_ID_REGISTERS, 0, opConfigData, 2);
}

/**
 * Issue a soft-reset to the BQ27441-G1A
 *
 * @return true on success
 * */
bool BQ27441_softReset(void) {
    return BQ27441_executeControlWord(BQ27441_CONTROL_SOFT_RESET);
}

void BQ27441_Full_Reset(void) {
    BQ27441_enterConfig(true);
    BQ27441_executeControlWord(BQ27441_CONTROL_RESET);
    BQ27441_exitConfig(true);

}

/**
 * Read a 16-bit command word from the BQ27441-G1A
 *
 * @param subAddress is the command to be read from
 * @return 16-bit value of the command's contents
 * */
static uint16_t BQ27441_readWord(uint16_t subAddress) {
    uint8_t data[2];
    BQ27441_i2cReadBytes(subAddress, data, 2);
    return ((uint16_t) data[1] << 8) | data[0];
}

/**
 * Read a 16-bit subcommand() from the BQ27441-G1A's control()
 *
 * @param function is the subcommand of control() to be read
 * @return 16-bit value of the subcommand's contents
 * */
static uint16_t BQ27441_readControlWord(uint16_t function) {

    uint8_t command[2] = {function & 0x00FF, function >> 8};
    uint8_t data[2] = {0, 0};

    BQ27441_i2cWriteBytes((uint8_t) 0, command, 2);

    if (BQ27441_i2cReadBytes((uint8_t) 0, data, 2)) {
        return ((uint16_t) data[1] << 8) | data[0];
    }

    return false;
}

/**
 * Execute a subcommand() from the BQ27441-G1A's control()
 *
 * @param function is the subcommand of control() to be executed
 * @return true on success
 * */
static bool BQ27441_executeControlWord(uint16_t function) {

    uint8_t command[2] = {function & 0x00FF, function >> 8};

    if (BQ27441_i2cWriteBytes((uint8_t) 0, command, 2))
        return true;

    return false;
}

/*****************************************************************************
 ************************** Extended Data Commands ***************************
 *****************************************************************************/

/**
 * Issue a BlockDataControl() command to enable BlockData access
 *
 * @return true on success
 * */
static bool BQ27441_blockDataControl(void)
{
    uint8_t enableByte = 0x00;
    return BQ27441_i2cWriteBytes(BQ27441_EXTENDED_CONTROL, &enableByte, 1);
}

/**
    Issue a DataClass() command to set the data class to be accessed

    @param id is the id number of the class
    @return true on success
*/
static bool BQ27441_blockDataClass(uint8_t id)
{
    return BQ27441_i2cWriteBytes(BQ27441_EXTENDED_DATACLASS, &id, 1);
}

/**
    Issue a DataBlock() command to set the data block to be accessed

    @param offset of the data block
    @return true on success
*/
static bool BQ27441_blockDataOffset(uint8_t offset)
{
    return BQ27441_i2cWriteBytes(BQ27441_EXTENDED_DATABLOCK, &offset, 1);
}

/**
 * Read the current checksum using BlockDataCheckSum()
 *
 * @return true on success
 * */
static uint8_t BQ27441_blockDataChecksum(void)
{
    uint8_t csum;
    BQ27441_i2cReadBytes(BQ27441_EXTENDED_CHECKSUM, &csum, 1);
    return csum;
}

/**
 * @return Use BlockData() to read a byte from the loaded extended data
 *
 * @param offset of data block byte to be read
 * @return true on success
 * */
static uint8_t BQ27441_readBlockData(uint8_t offset)
{
    uint8_t ret;
    uint8_t address = offset + BQ27441_EXTENDED_BLOCKDATA;
    BQ27441_i2cReadBytes(address, &ret, 1);
    return ret;
}

/**
 * @return Use BlockData() to write a byte to an offset of the loaded data
 *
 * @param offset is the position of the byte to be written
 * data is the value to be written
 * @return true on success
 * */
static bool BQ27441_writeBlockData(uint8_t offset, uint8_t data)
{
    uint8_t address = offset + BQ27441_EXTENDED_BLOCKDATA;
    return BQ27441_i2cWriteBytes(address, &data, 1);
}

/**
 * Read all 32 bytes of the loaded extended data and compute a
 * checksum based on the values.
 *
 * @return 8-bit checksum value calculated based on loaded data
 * */
static uint8_t BQ27441_computeBlockChecksum(void)
{
    uint8_t data[32];
    BQ27441_i2cReadBytes(BQ27441_EXTENDED_BLOCKDATA, data, 32);

    uint8_t csum = 0;
    for (uint8_t i = 0; i < 32; i++) {
        csum += data[i];
    }
    csum = 255 - csum;

    return csum;
}

/**
 * Use the BlockDataCheckSum() command to write a checksum value
 *
 * @param csum is the 8-bit checksum to be written
 * @return true on success
 * */
static bool BQ27441_writeBlockChecksum(uint8_t csum)
{
    return BQ27441_i2cWriteBytes(BQ27441_EXTENDED_CHECKSUM, &csum, 1);
}

/**
 * Read a byte from extended data specifying a class ID and position offset
 *
 * @param classID is the id of the class to be read from
 *          offset is the byte position of the byte to be read
 * @return 8-bit value of specified data
 * */
static uint8_t BQ27441_readExtendedData(uint8_t classID, uint8_t offset) {
    uint8_t retData = 0;
    if (!userConfigControl) BQ27441_enterConfig(false);

    if (!BQ27441_blockDataControl()) // // enable block data memory control
        return false; // Return false if enable fails
    if (!BQ27441_blockDataClass(classID)) // Write class ID using DataBlockClass()
        return false;

    BQ27441_blockDataOffset(offset / 32); // Write 32-bit block offset (usually 0)

    BQ27441_computeBlockChecksum(); // Compute checksum going in
    /*uint8_t oldCsum =*/ BQ27441_blockDataChecksum();
    retData = BQ27441_readBlockData(offset % 32); // Read from offset (limit to 0-31)

    if (!userConfigControl) BQ27441_exitConfig(true);

    return retData;
}

/**
 * Write a specified number of bytes to extended data specifying a
 * class ID, position offset.
 *
 * @param classID is the id of the class to be read from
 *          offset is the byte position of the byte to be read
 *          data is the data buffer to be written
 *          len is the number of bytes to be written
 * @return true on success
 * */
static bool BQ27441_writeExtendedData(uint8_t classID, uint8_t offset, uint8_t * data, uint8_t len) {
    if (len > 32)
        return false;

    if (!userConfigControl) BQ27441_enterConfig(false);

    if (!BQ27441_blockDataControl()) // // enable block data memory control
        return false; // Return false if enable fails
    if (!BQ27441_blockDataClass(classID)) // Write class ID using DataBlockClass()
        return false;

    BQ27441_blockDataOffset(offset / 32); // Write 32-bit block offset (usually 0)
    BQ27441_computeBlockChecksum(); // Compute checksum going in
    /*uint8_t oldCsum =*/ BQ27441_blockDataChecksum();

    // Write data bytes:
    for (int i = 0; i < len; i++) {
        // Write to offset, mod 32 if offset is greater than 32
        // The blockDataOffset above sets the 32-bit block
        BQ27441_writeBlockData((offset % 32) + i, data[i]);
    }

    // Write new checksum using BlockDataChecksum (0x60)
    uint8_t newCsum = BQ27441_computeBlockChecksum(); // Compute the new checksum
    BQ27441_writeBlockChecksum(newCsum);

    if (!userConfigControl) BQ27441_exitConfig(true);

    return true;
}


static uint8_t constrain(const uint8_t x, const uint8_t a, const uint8_t b) {
    if(x < a) {
        return a;
    }else if(b < x) {
        return b;
    }else
        return x;
}
