# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(zc624Bootloader C CXX ASM)

include(CheckGit.cmake)
CheckGitSetup()

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(zc624Bootloader 
    zc624Bootloader.cpp
    i2c.c
    CI2cSlave.cpp
    )

pico_set_program_name(zc624Bootloader "zc624Bootloader")
pico_set_program_version(zc624Bootloader "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(zc624Bootloader 1)
pico_enable_stdio_usb(zc624Bootloader 0)


pico_set_linker_script(zc624Bootloader ${CMAKE_CURRENT_SOURCE_DIR}/memmap.ld)


# Add the standard library to the build
target_link_libraries(zc624Bootloader
        pico_stdlib)

add_subdirectory(external/pico_i2c_slave)

# Add the standard include files to the build
target_include_directories(zc624Bootloader PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(zc624Bootloader 
        hardware_spi
        hardware_i2c
        hardware_flash
        git_version
        i2c_slave
        )

pico_add_extra_outputs(zc624Bootloader)
