cmake_minimum_required(VERSION 3.20.0)

set(APP_BIN "main" CACHE STRING "Application binary to build: main, uart, pwm_led, adc, usb, ble")

if(APP_BIN STREQUAL "uart")
  set(CONF_FILE
    prj.conf
    src/bin/uart.conf
  )
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(RhythmDoctorButton)

if(APP_BIN STREQUAL "main")
  target_sources(app PRIVATE
    src/main.cpp
    src/battery_adc.cpp
    src/button.cpp
    src/light.cpp
    src/keyboard/ble.cpp
    src/keyboard/ble_hid.c
    src/keyboard/usb.cpp
  )
elseif(APP_BIN STREQUAL "uart")
  target_sources(app PRIVATE src/bin/test_uart.cpp)
elseif(APP_BIN STREQUAL "pwm_led")
  target_sources(app PRIVATE src/bin/test_pwm_led.cpp src/light.cpp)
elseif(APP_BIN STREQUAL "adc")
  target_sources(app PRIVATE src/bin/test_adc.cpp src/battery_adc.cpp)
elseif(APP_BIN STREQUAL "usb")
  target_sources(app PRIVATE src/bin/test_usb.cpp src/keyboard/usb.cpp)
elseif(APP_BIN STREQUAL "ble")
  target_sources(app PRIVATE
    src/bin/test_ble.cpp
    src/keyboard/ble.cpp
    src/keyboard/ble_hid.c
  )
else()
  message(FATAL_ERROR "Unknown APP_BIN='${APP_BIN}'")
endif()

target_include_directories(app PRIVATE
  src
)
