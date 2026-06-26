# RT-Thread 移植说明

本目录预留为项目本地 RT-Thread 移植适配区域。

RT-Thread 源码以 git 子模块形式放在 `external/rt-thread`。

## 来源信息

- 库名称：RT-Thread 标准版
- 来源：RT-Thread 官方仓库 `https://github.com/RT-Thread/rt-thread.git`
- 版本：`v5.2.2`
- Commit：`ddf52e2cdd977f14fc04035c88672ac204aec713`
- 许可证：Apache-2.0
- 用途：用于 AT32F403ARGT7 固件的内核、调度器、IPC、堆、组件初始化，以及后续设备框架和网络组件。

## 后续动作

从 `external/rt-thread` 复制或引用所需的 RT-Thread 内核文件和 ARM Cortex-M4 移植文件到固件构建边界内，并加入工程构建。
