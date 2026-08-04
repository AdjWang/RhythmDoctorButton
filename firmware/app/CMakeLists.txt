cmake_minimum_required(VERSION 3.20.0)

set(APP_BIN "main" CACHE STRING "Application binary to build: main, uart, cmd, storage, pwm_led, adc, usb, ble, key")

if(APP_BIN STREQUAL "main" OR APP_BIN STREQUAL "usb")
  # Composed device of cdc usb and usb hid.
  set(CONF_FILE prj.conf dev_usb_hid.conf)
else()
  # Default device of cdc usb.
  set(CONF_FILE prj.conf dev_cdc_serial.conf)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(RhythmDoctorButton)

if(APP_BIN STREQUAL "main")
  target_sources(app PRIVATE
    src/bin/main.cpp
    src/battery_adc.cpp
    src/button.cpp
    src/cmd.cpp
    src/setting.cpp
    src/light.cpp
    src/keyboard/ble.cpp
    src/keyboard/ble_hid.c
    src/keyboard/usb.cpp
    src/keyboard/usb_device.cpp
  )
elseif(APP_BIN STREQUAL "uart")
  target_sources(app PRIVATE src/bin/test_uart.cpp)
elseif(APP_BIN STREQUAL "cmd")
  target_sources(app PRIVATE src/bin/test_cmd.cpp src/cmd.cpp src/setting.cpp)
elseif(APP_BIN STREQUAL "storage")
  target_sources(app PRIVATE src/bin/test_storage.cpp)
elseif(APP_BIN STREQUAL "pwm_led")
  target_sources(app PRIVATE src/bin/test_pwm_led.cpp src/light.cpp)
elseif(APP_BIN STREQUAL "key")
  target_sources(app PRIVATE
    src/bin/test_key.cpp
    src/button.cpp
    src/light.cpp
  )
elseif(APP_BIN STREQUAL "adc")
  target_sources(app PRIVATE src/bin/test_adc.cpp src/battery_adc.cpp)
elseif(APP_BIN STREQUAL "usb")
  target_sources(app PRIVATE
    src/bin/test_usb.cpp
    src/keyboard/usb.cpp
    src/keyboard/usb_device.cpp
  )
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
