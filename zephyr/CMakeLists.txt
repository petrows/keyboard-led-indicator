cmake_minimum_required(VERSION 3.13.1)

# Define board DeviceTree overbaly, lets use one for all
# https://docs.platformio.org/en/stable/frameworks/zephyr.html#devicetree-overlays
set(DTC_OVERLAY_FILE board.overlay)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(keyboard-led-indicator)

FILE(GLOB app_sources ../src/*.c*)
target_sources(app PRIVATE ${app_sources})
