# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

# add_compile_options(-Wall -Wextra -pedantic -Werror)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_PATH "<path to SDK>")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(zc95 C CXX ASM)

include(CheckGit.cmake)
CheckGitSetup()


# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(zc95 
  zc95.cpp 
  CLedControl.cpp 
  CLedControl.h 
  CControlsPortExp.cpp 
  CExtInputPortExp.cpp
  CEeprom.cpp
  CSavedSettings.cpp
  CChannel_types.cpp
  i2c_scan.cpp
  CTimingTest.cpp
  CHwCheck.cpp
  CHwCheckZc624.cpp
  version.cpp

  core1/Core1.cpp
  core1/Core1Messages.cpp
  core1/CPowerLevelControl.cpp
  core1/CRoutineOutputDebug.cpp
  core1/CRoutineOutputCore1.cpp

  core1/output/CChannelConfig.cpp
  core1/output/CFullChannelAsSimpleChannel.cpp

  core1/output/collar/CCollarComms.cpp
  core1/output/collar/CCollarChannel.cpp

  core1/output/mk312/C312bComms.cpp
  core1/output/mk312/C312bComms.h
  core1/output/mk312/COutput312b.cpp
  core1/output/mk312/COutput312b.h
  core1/output/mk312/C312bTxBuffer.cpp

  core1/output/zc1Output/CZC1ChannelFull.cpp
  core1/output/zc1Output/CZC1Comms.cpp

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
  core1/routines/CLogic1.cpp

  FpKnobs/CFpRotEnc.cpp
  FpKnobs/CRotEnc.cpp
  FpKnobs/CFpAnalog.cpp

  display/CMenu.cpp
  display/CDisplay.cpp
  display/CMainMenu
  display/CMenuRoutineSelection
  display/CMenuRoutineAdjust
  display/CMenuSettings
  display/CMenuChannelConfig
  display/CMenuCollarConfig
  display/CMenuCollarConfigSelected
  display/CMenuSettingLedBrightnes
  display/CMenuSettingPowerStep
  display/CMenuSettingRampUpTime
  display/CMenuSettingAbout
  display/COptionsList
  display/CHorzBarGraph
  )

# target_compile_options(zc95 PUBLIC -Werror -Wall -Wextra -Wnull-dereference)
target_compile_options(zc95 PUBLIC -Wall -Wnull-dereference)
 
pico_generate_pio_header(zc95 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(zc95 ${CMAKE_CURRENT_LIST_DIR}/collar433.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)


pico_set_program_name(zc95 "zc95")
pico_set_program_version(zc95 "1.0")

pico_enable_stdio_uart(zc95 1)
pico_enable_stdio_usb(zc95 0)



# Add the standard library to the build
target_link_libraries(zc95 pico_stdlib)

# Add any user requested libraries
target_link_libraries(zc95
        hardware_spi
        hardware_i2c
        hardware_dma
        hardware_pio
        hardware_adc
        pico_multicore
        hagl
        hagl_hal
        git_version
        )

pico_add_extra_outputs(zc95)

add_subdirectory(external/hagl)
add_subdirectory(external/hagl_hal)
#target_link_libraries(zc95 hagl hagl_hal)

target_compile_definitions(zc95 PRIVATE
  NDEBUG
# HAGL_HAL_USE_SINGLE_BUFFER
  HAGL_HAL_USE_DOUBLE_BUFFER
# HAGL_HAL_USE_TRIPLE_BUFFER
  HAGL_HAL_USE_DMA
  HAGL_HAL_DEBUG
  MIPI_DISPLAY_ST7735
)



target_compile_definitions(zc95 PRIVATE
  MIPI_DISPLAY_SPI_CLOCK_SPEED_HZ=48000000
 #MIPI_DISPLAY_SPI_CLOCK_SPEED_HZ=12000000
  MIPI_DISPLAY_PIN_CS=17
  MIPI_DISPLAY_PIN_DC=20
  MIPI_DISPLAY_PIN_RST=2
  MIPI_DISPLAY_PIN_BL=6
  MIPI_DISPLAY_PIN_CLK=18
  MIPI_DISPLAY_PIN_MOSI=19
  MIPI_DISPLAY_PIN_MISO=16
  MIPI_DISPLAY_PIXEL_FORMAT=MIPI_DCS_PIXEL_FORMAT_16BIT
  MIPI_DISPLAY_ADDRESS_MODE=MIPI_DCS_ADDRESS_MODE_SWAP_XY|MIPI_DCS_ADDRESS_MODE_MIRROR_Y
  MIPI_DISPLAY_WIDTH=160
  MIPI_DISPLAY_HEIGHT=128
  MIPI_DISPLAY_OFFSET_X=0
  MIPI_DISPLAY_OFFSET_Y=0
  MIPI_DISPLAY_INVERT=0
)
