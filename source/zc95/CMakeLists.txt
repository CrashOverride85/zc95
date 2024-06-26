# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)

# add_compile_options(-Wall -Wextra -pedantic -Werror)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_CYW43_SUPPORTED 1)

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_PATH "<path to SDK>")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(zc95 C CXX ASM)

include(CheckGit.cmake)
CheckGitSetup()

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()
set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)

add_custom_command(
  OUTPUT RemoteAccess/GattProfile.h
    DEPENDS RemoteAccess/GattProfile.gatt
    COMMAND ${BTSTACK_ROOT}/tool/compile_gatt.py
    ARGS ${CMAKE_SOURCE_DIR}/RemoteAccess/GattProfile.gatt ${CMAKE_SOURCE_DIR}/RemoteAccess/GattProfile.h
)

# Add executable. Default name is the project name, version 0.1
add_executable(zc95 
  CAnalogueCapture.cpp
  CBatteryGauge.cpp
  CChannel_types.cpp
  CDebugOutput.cpp
  CEeprom.cpp
  CExtInputPortExp.cpp
  CHwCheck.cpp 
  CLedControl.cpp 
  CLedControl.h 
  CLuaStorage.cpp
  CMainBoardPortExp.cpp 
  CRadio.cpp
  CSavedSettings.cpp
  CTimingTest.cpp
  CUtil.cpp
  FlashHelper.cpp
  globals.cpp
  gDebugCounters.cpp
  i2c_scan.cpp
  zc95.cpp

  AudioInput/CMCP4651.cpp
  AudioInput/CAudio.cpp
  AudioInput/CAudio3Process.cpp
  AudioInput/ESP32_fft.cpp

  Bluetooth/CBluetooth.cpp
  Bluetooth/CBluetoothScan.cpp
  Bluetooth/CBluetoothPair.cpp
  Bluetooth/CBluetoothConnect.cpp
  Bluetooth/CBluetoothRemote.cpp

  core1/Core1.cpp
  core1/Core1Messages.cpp
  core1/CPowerLevelControl.cpp
  core1/CRoutineOutputCore1.cpp
  core1/CRoutineOutput.cpp

  core1/output/CChannelConfig.cpp
  core1/output/CFullChannelAsSimpleChannel.cpp

  core1/output/collar/CCollarComms.cpp
  core1/output/collar/CCollarChannel.cpp

  core1/output/ZC624Output/CZC624ChannelFull.cpp
  core1/output/ZC624Output/CZC624Comms.cpp
 
  core1/routines/CAccPort.cpp

  core1/routines/CWaves.cpp
  core1/routines/CToggle.cpp
  core1/routines/CRoundRobin.cpp
  core1/routines/CTens.cpp
  core1/routines/CClimb.cpp
  core1/routines/CTriggeredClimb.cpp
  core1/routines/CFire.cpp
  core1/routines/CClimbPulse.cpp
  core1/routines/CShockChoice.cpp
  core1/routines/CCamTrigger.cpp
  core1/routines/CBuzz.cpp
  core1/routines/CPredicament.cpp
  core1/routines/CAudioThreshold.cpp
  core1/routines/CAudioWave.cpp
  core1/routines/CAudioIntensity.cpp
  core1/routines/CAudioVirtual3.cpp
  core1/routines/CLuaRoutine.cpp
  core1/routines/CDirectPulse.cpp
  
  display/CMenu.cpp
  display/CDisplay.cpp
  display/CMainMenu.cpp
  display/CMenuRoutineSelection.cpp
  display/CMenuRoutineAdjust.cpp

  display/COptionsList.cpp
  display/CHorzBarGraph.cpp
  display/CDisplayMessage.cpp

  display/config/CMenuSettings.cpp
  display/config/CMenuSettingAbout.cpp
  display/config/CMenuSettingAudio.cpp
  display/config/CMenuSettingHardware.cpp
  display/config/CMenuSettingOutput.cpp

  display/config/bluetooth_peripherals/CMenuBluetooth.cpp
  display/config/bluetooth_peripherals/CMenuBluetoothScan.cpp
  display/config/bluetooth_peripherals/CMenuBluetoothTest.cpp
  display/config/bluetooth_peripherals/CMenuBluetoothMap.cpp

  display/config/channel_config/CMenuChannelConfig.cpp
  display/config/channel_config/CMenuCollarConfig.cpp
  display/config/channel_config/CMenuCollarConfigSelected.cpp

  display/config/display_options/CMenuSettingDisplayOptions.cpp
  display/config/display_options/CMenuSettingButtonBrightness.cpp
  display/config/display_options/CMenuSettingPowerLevelDisplay.cpp
  display/config/display_options/CMenuSettingLedBrightnes.cpp

  display/config/remote_access/CMenuRemoteAccess.cpp
  display/config/remote_access/CMenuRemoteAccessConnectWifi.cpp
  display/config/remote_access/CMenuRemoteAccessSerial.cpp
  display/config/remote_access/CMenuApMode.cpp
  display/config/remote_access/CMenuRemoteAccessBLE.cpp
  display/config/remote_access/CMenuRemoteAccessBleConfig.cpp

  FrontPanel/CRotEnc.cpp
  FrontPanel/CFrontPanelV01.cpp
  FrontPanel/CFrontPanelV02.cpp
  
  LuaScripts/LuaScripts.S
  LuaScripts.cpp

  RemoteAccess/dhcpserver.c
  RemoteAccess/dnserver.c
  RemoteAccess/httpd.c
  RemoteAccess/CWifi.cpp
  RemoteAccess/setupwebinterface.cpp
  RemoteAccess/wlanscanner.cpp
  RemoteAccess/QR-Code-generator/c/qrcodegen.c
  RemoteAccess/strcasestr.c
  RemoteAccess/sha1.c
  RemoteAccess/base64.c
  RemoteAccess/CWebServer.cpp
  RemoteAccess/CWsConnection.cpp
  RemoteAccess/CLuaLoad.cpp
  RemoteAccess/CRoutineRun.cpp
  RemoteAccess/CMessageProcessor.cpp
  RemoteAccess/CSerialConnection.cpp
  RemoteAccess/CBtGatt.cpp
  ${PICO_SDK_PATH}/lib/lwip/src/apps/http/fs.c
  RemoteAccess/GattProfile.h
  )

# target_compile_options(zc95 PUBLIC -Werror -Wall -Wextra -Wnull-dereference)
target_compile_options(zc95 PUBLIC -Wall -Wnull-dereference)
 
pico_generate_pio_header(zc95 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
pico_generate_pio_header(zc95 ${CMAKE_CURRENT_LIST_DIR}/collar433.pio OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)


pico_set_program_name(zc95 "zc95")
pico_set_program_version(zc95 "1.0")

pico_enable_stdio_uart(zc95 1)
pico_enable_stdio_usb(zc95 0)



# Add the standard library to the build
target_link_libraries(zc95 pico_stdlib)

# Add any user requested libraries
target_link_libraries(zc95
        pico_cyw43_arch_lwip_poll
        pico_btstack_cyw43
        pico_btstack_ble
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_adc
        pico_multicore
        pico_float
        hardware_flash
        hagl
        hagl_hal
        git_version
        lua_static
        ArduinoJson
        )

pico_add_extra_outputs(zc95)

add_subdirectory(external/hagl)
add_subdirectory(external/hagl_hal)
add_subdirectory(external/lua)
add_subdirectory(external/ArduinoJson)
#target_link_libraries(zc95 hagl hagl_hal)

target_compile_definitions(zc95 PRIVATE
  NDEBUG
# HAGL_HAL_USE_SINGLE_BUFFER
  HAGL_HAL_USE_DOUBLE_BUFFER
# HAGL_HAL_USE_TRIPLE_BUFFER
  HAGL_HAL_USE_DMA
  HAGL_HAL_DEBUG
  MIPI_DISPLAY_ST7735
  PICO_STACK_SIZE=8192
  PICO_CORE1_STACK_SIZE=0 # stack is malloc'd
  PICO_USE_STACK_GUARDS   # would rather die instantly on stack overflow, than get weird, hard to debug, errors
  
  # For Lua. If a Lua script consumes all the memory, Lua can handle that and give an OOM error, which is better 
  # than crashing with a Panic. Ideally Lua would use a seperate alloc function that didn't panic, whilst everything
  # else did. Not quite sure how to manage that yet; multicore/malloc mutex complicates things.
  PICO_MALLOC_PANIC=0
)

target_compile_definitions(zc95 PRIVATE
  MIPI_DISPLAY_SPI_CLOCK_SPEED_HZ=48000000
 #MIPI_DISPLAY_SPI_CLOCK_SPEED_HZ=12000000
  MIPI_DISPLAY_PIN_CS=17
  MIPI_DISPLAY_PIN_DC=20
  MIPI_DISPLAY_PIN_RST=2
  MIPI_DISPLAY_PIN_BL=-1
  MIPI_DISPLAY_PIN_CLK=18
  MIPI_DISPLAY_PIN_MOSI=19
  MIPI_DISPLAY_PIN_MISO=16
  MIPI_DISPLAY_PIN_POWER=-1
  MIPI_DISPLAY_PIN_TE=-1
  MIPI_DISPLAY_SPI_PORT=spi0
  MIPI_DISPLAY_PIXEL_FORMAT=MIPI_DCS_PIXEL_FORMAT_16BIT
  MIPI_DISPLAY_ADDRESS_MODE=MIPI_DCS_ADDRESS_MODE_SWAP_XY|MIPI_DCS_ADDRESS_MODE_MIRROR_Y
  MIPI_DISPLAY_WIDTH=160
  MIPI_DISPLAY_HEIGHT=128
  MIPI_DISPLAY_OFFSET_X=0
  MIPI_DISPLAY_OFFSET_Y=0
  MIPI_DISPLAY_INVERT=0

  PICO_DEFAULT_UART_TX_PIN=8
  PICO_DEFAULT_UART_RX_PIN=9
  CYW43_HOST_NAME="ZC95"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/RemoteAccess
    ${PICO_SDK_PATH}/lib/lwip/include/lwip/apps
)
