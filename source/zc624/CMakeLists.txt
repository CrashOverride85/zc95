# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_SDK_PATH "<sdk path>")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(OutputZc C CXX ASM)

include(CheckGit.cmake)
CheckGitSetup()

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(OutputZc 
        OutputZc.cpp
        CMCP4728.cpp
        COutputChannel.cpp
        COutput.cpp
        CMessageProcess.cpp
        CMsgDac.cpp
        CPulseQueue.cpp
        CI2cSlave.cpp
)

pico_generate_pio_header(OutputZc ${CMAKE_CURRENT_LIST_DIR}/pulse_gen.pio OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)


pico_set_program_name(OutputZc "OutputZc")
pico_set_program_version(OutputZc "0.1")

pico_enable_stdio_uart(OutputZc 1)
pico_enable_stdio_usb(OutputZc 0)

# Add the standard library to the build
target_link_libraries(OutputZc pico_stdlib)

# Add any user requested libraries
target_link_libraries(OutputZc
        hardware_spi
        hardware_i2c
        hardware_pio
        hardware_adc
        pico_multicore
        i2c_slave
        git_version
        )

pico_add_extra_outputs(OutputZc)
add_subdirectory(external/pico_i2c_slave)
