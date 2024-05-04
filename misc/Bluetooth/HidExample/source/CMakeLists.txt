# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)


# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_PATH "<path to SDK>")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.1.1")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(hid_example C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(hid_example 
  hid_example.cpp 
  bluetooth_send.cpp
)

pico_set_program_name(hid_example "hid_example")
pico_set_program_version(hid_example "0.1")

pico_enable_stdio_uart(hid_example 1)
pico_enable_stdio_usb(hid_example 0)

# Add the standard library to the build
target_link_libraries(hid_example
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(hid_example PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(hid_example 
        hardware_adc
        pico_cyw43_arch_lwip_poll
        pico_btstack_cyw43
        pico_btstack_ble
        )

pico_add_extra_outputs(hid_example)


