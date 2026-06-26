# AT32F403A_407 firmware library source notice

- Original library: AT32F403A_407 Firmware Library
- Version: V2.2.2
- Source: existing project `external/AT32F403A_407_Firmware_Library_V2.2.2`
- Vendor: Artery Technology
- Copy date: 2026-06-15
- Local modifications: no source modifications in `firmware/vendor` at initial copy
- Target MCU: AT32F403ARGT7
- Use in project: CMSIS device support, GCC startup file, and AT32F403A_407 peripheral drivers for GCC/CMake builds
- Additional copy on 2026-06-22: `libraries/cmsis/cm4/device_support/startup/gcc/startup_at32f403a_407.s` copied from the same upstream `external` library for the CMake/GCC target.
- Local modification on 2026-06-22: GCC startup `Reset_Handler` calls RT-Thread `entry` instead of application `main`, so board/kernel startup runs before the application main thread.

The files under this directory are a project-local build copy. Keep the
upstream copy under `external` unchanged.
