# RT-Thread port integration

This directory reserves the project-local RT-Thread port integration area.

RT-Thread source is tracked as a git submodule under `external/rt-thread`.

Source information:

- Library: RT-Thread standard edition
- Source: official RT-Thread repository, `https://github.com/RT-Thread/rt-thread.git`
- Version: `v5.2.2`
- Commit: `ddf52e2cdd977f14fc04035c88672ac204aec713`
- License: Apache-2.0
- Intended use: kernel, scheduler, IPC, heap, component initialization, and
  later device/network components for the AT32F403ARGT7 firmware

Next step: copy or reference the required RT-Thread kernel and ARM Cortex-M4
port files from `external/rt-thread` into the firmware build boundary and add
them to the Keil project.
