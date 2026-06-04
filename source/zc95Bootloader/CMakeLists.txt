# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(zc95Bootloader C CXX ASM)

include(CheckGit.cmake)
CheckGitSetup()

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(zc95Bootloader 
    zc95Bootloader.c 
    sdcard/hwconfig.c
    sdcard/sd_card.c
    xmodem/xmodem.c
    buffered_serial.c
    leds.c
    eeprom.c
    i2c.c
    zc624.c
    zc_debug.c
    zc_types.c
    zc_firmware.c
    )

pico_set_program_name(zc95Bootloader "zc95Bootloader")
pico_set_program_version(zc95Bootloader "0.1")

# no_flash means the target is to run from RAM
#pico_set_binary_type(zc95Bootloader)

# Generate PIO header
pico_generate_pio_header(zc95Bootloader ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(zc95Bootloader 1)
pico_enable_stdio_usb(zc95Bootloader 0)


pico_set_linker_script(zc95Bootloader ${CMAKE_CURRENT_SOURCE_DIR}/memmap.ld)

add_subdirectory(sdcard/FatFs_SPI build)

# Add the standard library to the build
target_link_libraries(zc95Bootloader
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(zc95Bootloader PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(zc95Bootloader 
        hardware_spi
        hardware_i2c
        hardware_pio
        hardware_flash
        FatFs_SPI
        git_version
        )

pico_add_extra_outputs(zc95Bootloader)

target_compile_definitions(zc95Bootloader PRIVATE

)
