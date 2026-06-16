# Firmware project layout

This directory contains the files intended to participate in the firmware build.

- `app`: application entry and future device business logic
- `bsp/at32f403`: AT32F403ARGT7 board support and RT-Thread board hooks
- `external_port`: project ports for external middleware
- `docs`: firmware bring-up notes and verified hardware mappings
- `project/mdk_v5`: Keil MDK project files
- `vendor`: copied third-party or vendor source used by the build

Current status:

- AT32F403A_407 firmware library V2.2.2 has been copied from `external` into `vendor`.
- Target MCU is confirmed as AT32F403ARGT7.
- The earlier `AT32F403_Firmware_Library_V2.1.3` vendor copy is kept for traceability but is not used by the current Keil project.
- RT-Thread source is tracked as submodule `external/rt-thread` at `v5.2.2`.
- Minimal RT-Thread v5.2.2 kernel files have been copied into `vendor/rt-thread-v5.2.2` and are referenced by the Keil project.
- Hardware-specific peripheral initialization currently covers the verified debug UART, LEDs, keys, a minimal ST7789 hardware SPI2 screen bring-up driver with basic drawing primitives, HDC1080, and AT24C128C EEPROM.
- `drivers/sensor` contains the project-local HDC1080 I2C1 bring-up driver for the interaction board humidity/temperature sensor.
- `drivers/storage` contains the project-local AT24C128C I2C2 EEPROM bring-up driver for the 16 KB external parameter/event storage device.
- `app_data_service` provides the first backend data service thread and device data snapshot boundary. It currently runs as `data_svc` with priority 18, stack 768 bytes, 200 ms update period, and reads HDC1080 environment temperature/humidity every 1000 ms. HDC1080 data is stored as raw, single-point calibrated, and filtered snapshot values.
- `app_ui` is the current front-end display layer. It renders data-service snapshot data only and does not access ADC, Flash, Modbus, or hardware sampling directly.
- `app_key_input` scans and debounces the verified front-panel keys, then reports front-end key events to the UI layer.
- `app_bringup` wraps hardware bring-up commands for LEDs, keys, and LCD tests so `app_main` stays focused on the main loop and CLI dispatch.

Bring-up documents:

- `docs/gpio_mapping.md`: verified GPIO mapping for debug UART, LEDs, keys, and the current hardware SPI2 ST7789 screen path.
