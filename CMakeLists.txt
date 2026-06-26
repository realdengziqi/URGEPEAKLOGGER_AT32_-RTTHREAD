cmake_minimum_required(VERSION 3.22)

project(SurgePeakLogger_AT32_rtthread C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(firmware/cmake/at32_targets.cmake)

surge_add_firmware_target(surge_backend_serial)

