# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")
cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(Pico_ePaper_73)
pico_sdk_init()

# add a compilation subdirectory
add_subdirectory(lib/Config)
add_subdirectory(lib/e-Paper)
add_subdirectory(lib/RTC)
add_subdirectory(lib/Fonts)
add_subdirectory(lib/GUI)
add_subdirectory(lib/led) 
add_subdirectory(lib/minIni)
add_subdirectory(examples)
add_subdirectory(lib/FatFs_SPI build)

# add a header directory
include_directories(examples)
include_directories(./lib/Config)
include_directories(./lib/GUI)
include_directories(./lib/RTC)
include_directories(./lib/led)
include_directories(./lib/minIni)

# generate an executable file
add_executable(Pico_ePaper_73
main.c
)

# enable usb output, disable uart output
pico_enable_stdio_usb(Pico_ePaper_73 1)
pico_enable_stdio_uart(Pico_ePaper_73 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(Pico_ePaper_73)
 
target_link_libraries(Pico_ePaper_73 examples ePaper GUI led Fonts Config RTC FatFs_SPI minIni)