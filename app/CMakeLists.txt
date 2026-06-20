cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(RhythmDoctorButton)

target_sources(app PRIVATE
  src/main.cpp
  src/battery_adc.cpp
  src/button.cpp
  src/light.cpp
  src/keyboard/ble.cpp
  src/keyboard/ble_hid.c
  src/keyboard/usb.cpp
)
target_include_directories(app PRIVATE
  src
)
