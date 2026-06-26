# RT-Thread Source Notice

- Original library: RT-Thread standard edition
- Version: v5.2.2
- Commit: ddf52e2cdd977f14fc04035c88672ac204aec713
- Source: external/rt-thread submodule, upstream https://github.com/RT-Thread/rt-thread.git
- License: Apache-2.0
- Copy date: 2026-06-15
- Intended use: minimum RT-Thread kernel boot set for AT32F403ARGT7 firmware
- Local modifications:
  - components/finsh/shell.c: add a 5 ms delay when the project non-blocking
    console getter returns no data, preventing the MSH thread from busy-waiting.
  - components/finsh/shell.c: ignore the LF immediately after a CR so CRLF
    serial clients do not execute an empty command and print a duplicate prompt.

Copied source set:

- include/*
- components/libc/compilers/common/extension/sys/types.h copied to include/sys/types.h
- components/libc/compilers/common/extension/sys/errno.h copied to include/sys/errno.h
- components/libc/compilers/common/include/sys/signal.h copied to include/sys/signal.h
- src/clock.c
- src/components.c
- src/cpu_up.c
- src/defunct.c
- src/idle.c
- src/ipc.c
- src/irq.c
- src/kservice.c
- src/mem.c
- src/object.c
- src/scheduler_comm.c
- src/scheduler_up.c
- src/thread.c
- src/timer.c
- src/klibc/kstring.c
- src/klibc/kstdio.c
- src/klibc/kerrno.c
- src/klibc/rt_vsnprintf_tiny.c
- src/klibc/rt_vsscanf.c
- libcpu/arm/cortex-m4/cpuport.c
- libcpu/arm/cortex-m4/context_rvds.S
- libcpu/arm/cortex-m4/context_gcc.S copied on 2026-06-22 for the CMake/GCC target
- components/finsh/finsh.h
- components/finsh/msh.c
- components/finsh/msh.h
- components/finsh/shell.c
- components/finsh/shell.h

Not copied in this stage: DFS, lwIP, SAL, netdev, device drivers, tests, SMP scheduler, and Smart/lwp files.
