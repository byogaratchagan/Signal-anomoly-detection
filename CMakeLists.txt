cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
set(CMAKE_OBJECT_PATH_MAX 500)

# Find the pico-extras package
# (This line requires you to set the PICO_EXTRAS_PATH environment variable)

project(pico-projects)

pico_sdk_init()
 
add_executable(main main.cpp)


add_subdirectory(pico-tflmicro)
target_link_libraries(main pico-tflmicro)

target_link_libraries(main 
                    pico_stdio
                    pico_stdlib
                    hardware_pio
                    hardware_dma
                    hardware_adc
                    hardware_timer
                    hardware_clocks
                    )

pico_generate_pio_header(main ${CMAKE_CURRENT_LIST_DIR}/shift_reg.pio)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_add_extra_outputs(main)