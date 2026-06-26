# FreeModbus source notice

- Original library: RT-Thread-packages/freemodbus
- Source URL: https://github.com/RT-Thread-packages/freemodbus.git
- Copied commit: 18b2228605c1aa3ad9145147f0eaa3270adf5012
- License: BSD-style license, see `LICENSE`
- Copied date: 2026-06-22
- Local changes: core `modbus` sources are copied as-is. Board-specific port and register mapping are implemented outside this vendor copy under `firmware/external_port/freemodbus_port` and `firmware/app`.

This directory is the buildable vendor copy used by the firmware. The upstream
git submodule remains under `external/freemodbus`.
