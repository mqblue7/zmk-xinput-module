cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_ZMK_XINPUT)
  zephyr_library()
  zephyr_library_sources(src/xinput_usb.c)
  zephyr_library_sources(src/behavior_xinput.c)
  zephyr_include_directories(include)
endif()
