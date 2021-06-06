cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pimoroni_tiny2040")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(/agx_ssd/tiny2040/SDK/pico/pico-examples/pico_sdk_import.cmake)

project(Thermometer C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of the project
add_executable(temp
  ../../Display/sh1107/draw_graphics.c
  ../../Display/sh1107/draw_char.c
  ../../Display/sh1107/pixel_ops.c
  ../../Display/sh1107/sh1107_spi.c
  ../../Display/sh1107/blink.c
  OneWire.c
  DS18B20.c
  )

pico_generate_pio_header(temp ${CMAKE_CURRENT_LIST_DIR}/OneWire.pio)

target_sources(temp PRIVATE 
    OneWire.c 
    DS18B20.c
    )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(temp PRIVATE
      pico_stdlib 
      hardware_spi 
      hardware_timer
      hardware_pio
      )

# create map/bin/hex file etc.
pico_add_extra_outputs(temp)

# add url via pico_set_program_url
#example_auto_set_url(temp)
