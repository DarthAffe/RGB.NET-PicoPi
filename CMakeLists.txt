cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_SDK_PATH "../../pico-sdk")

include(pico_sdk_import.cmake)

project(RGB.NET C CXX ASM)

pico_sdk_init()

add_executable(RGB.NET)

pico_generate_pio_header(RGB.NET ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

target_include_directories(RGB.NET PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_sources(RGB.NET PRIVATE RGB.NET.c usb_descriptors.c)

pico_set_program_name(RGB.NET "RGB.NET")
pico_set_program_version(RGB.NET "1.0")

pico_enable_stdio_uart(RGB.NET 0)
pico_enable_stdio_usb(RGB.NET 0)

target_link_libraries(RGB.NET PRIVATE pico_stdlib hardware_pio pico_multicore tinyusb_device tinyusb_board hardware_flash)

pico_add_extra_outputs(RGB.NET)
