# 项目概述

本项目是一个基于 **AT32F403A** 的嵌入式浪涌峰值记录器固件项目。

设备功能包括：

- AT32F403A 主控
- 240x240 小尺寸彩屏
- RS485 接口，支持 Modbus RTU
- 以太网接口，支持 Modbus TCP
- 开关量输入检测
- 外接 NTC 温度检测
- 环境温湿度检测
- RTC / 时间记录
- 浪涌峰值事件记录
- 参数存储
- 事件记录查询
- 可能包含按键、蜂鸣器、LED、报警输出等功能

本项目应按工业嵌入式仪表固件处理，不应按简单 Demo 项目处理。

# 辅助资料

- `datasheet` 目录里有我提供的一些硬件数据手册和参考信息。有一些芯片的手册我没有收集在里面，如果后面经常需要查阅手册，可以将这些手册也保存在 `datasheet` 目录中。
- `hardware_info` 里有两个板子的资料。
- `competing_product_info` 里有竞品的资料。

新增 datasheet 前，必须说明资料来源、芯片型号、版本和用途，不要保存来源不明或与项目无关的资料。

# 语言和回复要求

- 默认使用中文解释。
- 新增或修改项目文档时默认使用中文；第三方原始许可证、上游 README 等需要保持原文的文件除外。
- 代码变量、函数名、文件名优先使用英文。
- 注释可以使用中文，尤其是硬件逻辑、采样逻辑、校准逻辑、产品业务逻辑。
- 每次修改后必须说明：
  - 改了哪些文件
  - 为什么这么改
  - 可能影响哪些功能
  - 是否需要硬件验证
- 对不确定的硬件条件，不要擅自猜测，必须明确标注为“待确认”。

# RTOS 选型要求

本项目目标 RTOS 为 **标准版 RT-Thread**，不优先使用 RT-Thread Nano。

选择标准版 RT-Thread 的原因：

- 本项目不是简单控制板，而是工业嵌入式仪表固件。
- 项目包含 LCD、RS485、以太网、Modbus TCP、参数存储、事件记录、RTC、传感器和调试接口等多个模块。
- 标准版 RT-Thread 更适合后续接入设备框架、FinSH、DFS、网络组件、lwIP、SAL/netdev 等功能。
- RT-Thread Nano 更适合极简调度场景，本项目不按 Nano 项目规划。

基本原则：

- 优先基于 RT-Thread 官方或 RT-Thread Studio 的 **AT32F403A AT-START BSP** 进行适配。
- 不要把 AT-START-F403A 开发板配置直接当作本项目硬件配置使用。
- UART、ADC、LCD、RS485、ETH、RTC、Flash 分区、外设引脚、PHY 地址等必须按本项目原理图确认。
- 可以使用标准版 RT-Thread 的内核、组件和设备框架，但不要为了“标准化”而一次性大规模重构全部外设驱动。
- AT32 外设驱动可以继续基于 AT32 官方固件库实现，再按项目需要逐步适配到 RT-Thread device 框架。

# 外置资源要求

项目要用到：

- 标准版 RT-Thread
- uGUI
- FreeModbus
- AT32 官方固件库

第三方框架统一放到 `external` 目录。

但是不要擅自升级、替换或重新 `git clone` 第三方库。
如果确实需要新增或更新第三方库，必须先说明原因、来源、版本、许可证，并等待确认。

`external` 目录中的内容视为上游原始副本，原则上不要直接修改。
需要参与编译的文件，可以复制到 `firmware/vendor` 或 `firmware/external_port` 目录中，并保留来源说明。

从 `external` 复制到 `firmware` 的第三方源码，必须保留来源说明，包括：

- 原始库名称
- 版本号或 commit
- 许可证
- 复制日期
- 是否做过本地修改

# 项目文件要求

所有要参与项目编译的文件都应该放到 `firmware` 目录里。也就是说 `external` 里的文件如果要用到，需要先复制到 `firmware` 里再添加到 Keil 工程。

推荐结构：

```text
project/
  AGENTS.md
  datasheet/
  hardware_info/
  competing_product_info/
  external/
  firmware/
    app/
    bsp/
    drivers/
    middleware/
    protocol/
    vendor/
    external_port/
  tools/
    build.ps1
    flash.ps1
```

如果现有工程结构不同，优先遵守现有结构，不要为了目录规范做大规模搬迁。

# 编译与验证要求

修改代码后，优先执行：

```powershell
.\tools\build.ps1
```

当前 `tools` 目录中的构建脚本：

- `tools/build.ps1`：Keil MDK 命令行构建脚本。
  - 默认 Target：`surge_rtthread_base_ac5`
  - Keil 工程：`firmware/project/mdk_v5/surge_rtthread_base.uvprojx`
  - 构建日志：`firmware/project/mdk_v5/<Target>_build.log`
  - 默认执行 Keil `-r` 重新构建。
  - 使用 `.\tools\build.ps1 -Build` 时执行 Keil `-b` 构建。
  - 可用 `.\tools\build.ps1 -Target <KeilTargetName>` 指定其他 Keil Target。
  - 脚本会自动查找 `C:\Keil_v5\UV4\UV4.exe` 或 `C:\Keil\UV4\UV4.exe`。
- `tools/flash.ps1`：Keil MDK 命令行烧录脚本。
  - 默认 Target：`surge_rtthread_base_ac5`
  - Keil 工程：`firmware/project/mdk_v5/surge_rtthread_base.uvprojx`
  - 烧录日志：`firmware/project/mdk_v5/<Target>_flash.log`
  - 使用 Keil `-f` 执行 Flash Download。
  - 使用绝对工程路径和绝对日志路径，避免 UV4 因相对 `-o` 路径弹出 “Create File -o ... failed”。
  - 当前 Keil 工程已配置 J-Link / SWD 下载，目标芯片为 `-AT32F403ARGT7`。

如果 `tools/build.ps1` 不存在，则优先使用 `build-keil` skill 查找 Keil `.uvprojx` 工程并编译。
如果 `tools/flash.ps1` 不存在，则优先使用 `flash-keil` skill；调用 `flash-keil` 时日志路径应使用绝对路径。

当前硬件调试串口：

- `COM12`：JLink CDC UART Port，对应主控板 `USART1` debug UART。
  - MCU 引脚：`PA9 USART1_TX_DEBUG`、`PA10 USART1_RX_DEBUG`
  - 默认参数：`115200 8N1`
  - 当前最小启动工程通过 `rt_hw_console_output()` 将 RT-Thread console 输出到该串口。

如果自动编译条件不具备，必须明确说明“未编译验证”，不要在没有编译结果的情况下声称“已验证通过”。

编译失败时：

- 先读取完整 build log。
- 优先修复根因，不要只掩盖错误。
- 不要为了通过编译而删除功能代码。
- 不要擅自修改 startup file、linker script、Keil Target、系统时钟配置。

涉及以下内容时，编译通过后还应建议硬件验证：

- ADC 采样
- RS485 / Modbus RTU
- Modbus TCP
- LCD 显示
- Flash 参数存储
- Flash 事件记录
- RTC 时间
- 以太网链路、PHY、lwIP、TCP 连接
- RT-Thread 线程优先级、栈大小、tick 配置和堆内存使用

# 标准版 RT-Thread 使用要求

本项目使用标准版 RT-Thread，但不要为了“看起来像 RTOS 项目”而过度拆分线程。

## 基本要求

- 不要在 ISR 中调用会阻塞、会调度等待或只允许在线程上下文使用的 RT-Thread API。
- ISR 中只允许使用 RT-Thread 文档明确支持中断上下文、且不会阻塞的 API。
- 中断中不要做复杂业务处理，复杂处理必须通过 event、semaphore、mailbox 或 message queue 通知线程侧完成。
- 新增线程时必须说明线程用途、优先级、栈大小、时间片配置。
- 注意 RT-Thread 优先级通常是数值越小优先级越高，修改优先级时必须避免和系统线程、空闲线程冲突。
- 不要在高优先级线程中长时间阻塞。
- 不要使用无延时、无阻塞、无事件等待的死循环线程。
- 线程间通信优先使用 message queue、mailbox、event、semaphore、mutex。
- 共享数据必须通过 mutex、临界区、消息队列、mailbox 或双缓冲快照保护。
- 修改线程优先级、tick rate、heap 配置、`rtconfig.h`、`RT_TICK_PER_SECOND`、`RT_THREAD_PRIORITY_MAX`、`RT_USING_HEAP` 等配置时必须先说明原因。
- 不要在线程中直接使用长时间 busy wait。
- 不要在高频线程中输出大量日志。
- 对新增线程，应考虑栈余量检查、异常处理和线程退出策略。

## 标准版组件使用原则

- 可以使用 FinSH / MSH 做调试命令，但不要让调试命令直接破坏业务数据一致性。
- 本项目正式接入 MSH 后，MSH 只作为开发阶段的模块测试、人工调试和 Codex 辅助开发入口，不作为产品正式用户接口或业务功能入口。
- MSH 命令属于前端调试接口，必须遵守前后端分离边界：读取状态应通过 `data_service_get_snapshot()`、`data_service_get_datetime()` 等统一接口，修改参数、时间、记录、报警等状态应提交给对应 service 处理。
- MSH 命令不得直接写 Flash 参数区、事件记录区、校准参数、Modbus 寄存器表或共享业务结构体；不得绕过 `param_service`、`record_service`、`data_service` 等后台服务直接改业务数据。
- MSH 命令可以保留硬件 bring-up 类直连命令，例如 GPIO、LED、LCD、按键、RTC 原始验证，但这些命令必须明确用于开发测试，不能作为正式业务路径。
- MSH 命令输出应控制频率，默认不做高频持续刷屏；需要 watch/monitor 类命令时必须支持关闭，并避免影响采样、通信、记录和 UI 线程。
- 可以使用 RT-Thread device framework，但不要求一次性把所有 AT32 外设驱动都改成标准 device 驱动。
- 可以使用 lwIP、SAL、netdev 等网络组件，但以 Modbus TCP 稳定为优先目标。
- 可以使用 DFS 或轻量文件系统，但 Flash 参数存储和事件记录布局必须先确认，不要为了引入文件系统破坏已有存储规划。
- 标准版 RT-Thread 组件应按项目需要逐步启用，不要一次性打开大量暂时不用的组件。
- 修改 `rtconfig.h`、Kconfig、SConscript、Keil 工程分组、组件开关时，必须说明原因和影响。

## BSP 适配原则

- 优先参考 `bsp/at32/at32f403a-start` 或 RT-Thread Studio 的 AT32F403A AT-START BSP。
- 参考 BSP 只能作为启动、时钟、串口、基础内核移植参考。
- 本项目硬件引脚、外设编号、PHY 地址、RS485 DE/RE 极性、LCD 接口、ADC 通道、RTC 时钟源必须按 `hardware_info` 和原理图确认。
- 不要擅自沿用开发板 LED、按键、串口、以太网 PHY、晶振频率等配置。
- 如果需要从参考 BSP 复制源码，必须保留来源说明、版本或 commit、复制日期和是否本地修改。

## 线程职责建议

推荐将线程职责划清：

- 采样线程只负责采样调度或低速采样。
- 数据服务线程负责维护设备状态。
- UI 线程只负责显示和菜单交互。
- Modbus RTU / TCP 线程只负责协议收发和请求转发。
- 记录线程负责 Flash 写入和历史记录维护。
- 传感器线程负责 NTC、温湿度等低速传感器读取。
- 按键线程负责按键扫描、消抖和输入事件上报。

# uGUI 使用要求

- uGUI 用于 240x240 小屏 UI。
- 不要在中断中调用 uGUI 绘图函数。
- 不要高频全屏刷新。
- 优先使用页面状态机、局部刷新、脏标志刷新。
- UI 只显示数据，不直接操作采样、存储和通信底层逻辑。
- UI 刷新不能阻塞采样、Modbus RTU、Modbus TCP 和事件记录。
- UI 页面逻辑和业务数据逻辑要分离。

# FreeModbus 使用要求

- FreeModbus 主要用于 Modbus RTU。
- Modbus TCP 可使用独立 TCP 适配层，但必须共用同一套寄存器映射。
- Modbus 寄存器映射必须集中管理。
- 不要把寄存器地址散落在多个文件中。
- 不要随意修改已有寄存器地址含义。
- RS485 UART 不能同时输出 debug log。
- 修改寄存器表后，必须同步更新文档。
- Modbus RTU 和 Modbus TCP 都属于前端协议适配层，不要直接访问 ADC、Flash、LCD 底层驱动。

推荐结构：

```text
modbus_rtu_thread
modbus_tcp_thread
  ↓
modbus_map.c / modbus_map.h
  ↓
data_service / param_service / record_service
```

# 本地可用 skills

## 小智学长 skills

本地环境已安装或可使用 embed-ai-tool 技能集：

```text
https://github.com/LeoKemp223/embed-ai-tool
```

优先使用以下 skill：

- `build-keil`：用于 Keil 工程编译
- `flash-keil`：用于 Keil 烧录
- `flash-jlink`：用于 J-Link 烧录
- `serial-monitor`：用于串口日志监控
- `modbus-debug`：用于 Modbus RTU / Modbus TCP 调试
- `memory-analysis`：用于分析 map / elf 内存占用
- `rtos-debug`：用于 RT-Thread 线程、栈余量、死锁风险分析
- `static-analysis`：用于静态检查
- `workflow`：用于编译、烧录、监控流水线

使用规则：

- 修改代码后，优先调用 `build-keil` 编译。
- 编译通过后，如用户要求，再调用 `flash-keil` 或 `flash-jlink` 烧录。
- 烧录后，优先调用 `serial-monitor` 查看启动日志。
- 修改 Modbus 相关代码后，优先调用 `modbus-debug` 做寄存器读写测试。
- 使用 RT-Thread 后，涉及线程、message queue、mailbox、event、semaphore、mutex、栈大小的问题，可以调用 `rtos-debug`。
- 不要使用 `stm32-hal-development` 直接改写 AT32 官方库代码。

# 禁止随意修改的内容

除非明确要求，不要随意修改：

- MCU 型号
- startup file
- linker script
- system clock / PLL 配置
- Keil Target 配置
- AT32 官方固件库源码
- RT-Thread 内核源码
- uGUI 原始源码
- FreeModbus 原始源码
- Modbus 寄存器地址
- Flash 参数存储布局
- Flash 事件记录布局
- ADC 通道映射
- RS485 UART 编号和 DE/RE 极性
- LCD 接口和引脚
- RTC 时钟源
- 以太网 PHY 地址
- 编译器版本和优化等级
- option bytes
- 读保护配置

# 软件架构要求

本项目使用标准版 RT-Thread，整体软件架构应采用类似“前后端分离”的思路。

这里的“后端”不是网络服务器，而是指设备内部的核心数据服务层；“前端”是指所有对外展示、对外通信、对外交互的模块。

## 架构核心思想

本项目应尽量分为：

```text
硬件采集层
  ↓
后台数据服务层 Data Service
  ↓
前端交互层：UI / Modbus RTU / Modbus TCP / 按键 / 调试接口
```

其中：

- 后台数据服务层负责维护设备的核心状态。
- UI、Modbus RTU、Modbus TCP 都属于前端。
- 前端模块不应该直接操作底层采样、Flash 记录、校准参数和硬件驱动。
- 前端模块应通过统一的数据接口访问后台数据服务。

## 后台数据服务层

后台数据服务层建议由一个或多个 RT-Thread 线程组成，例如：

```text
data_service_thread
record_service_thread
param_service_thread
alarm_service_thread
```

后台数据服务层负责：

- 管理实时测量数据
- 管理浪涌峰值事件
- 管理历史记录
- 管理设备参数
- 管理校准参数
- 管理报警状态
- 管理时间状态
- 向 UI、Modbus RTU、Modbus TCP 提供统一数据快照
- 处理来自前端的参数修改请求、记录查询请求、清除记录请求等

后台数据服务层是设备业务逻辑的中心。

## 前端模块定义

以下模块都视为“前端”：

```text
UI 页面
Modbus RTU
Modbus TCP
按键菜单
调试串口命令
后续可能增加的 Web 配置页面
```

前端模块只负责：

- 展示数据
- 查询数据
- 提交用户操作
- 提交通信请求
- 显示状态
- 返回协议响应

前端模块不应该直接实现核心业务逻辑。

## 数据访问规则

UI、Modbus RTU、Modbus TCP 不要分别维护自己的数据副本。

推荐方式：

```text
data_service_get_snapshot()
data_service_get_record()
data_service_set_param()
data_service_get_param()
data_service_clear_alarm()
data_service_clear_records()
```

前端模块应通过这些统一接口访问数据。

禁止出现以下情况：

- UI 直接读取 ADC 原始数据
- Modbus RTU 直接访问 ADC 驱动
- Modbus TCP 直接写 Flash
- UI 直接修改 Flash 参数区
- 多个模块各自维护一份报警状态
- 多个模块各自计算浪涌峰值
- 多个模块各自维护时间有效标志
- Modbus RTU 和 Modbus TCP 各自维护不同寄存器表

## 数据快照机制

实时数据建议通过快照方式提供给前端。

推荐设计：

```c
typedef struct
{
    uint32_t timestamp;
    uint8_t time_valid;

    uint16_t surge_peak_raw;
    int32_t surge_peak_value_milli;

    int32_t ntc_temp_milli_c;
    int32_t env_temp_milli_c;
    int32_t env_humi_milli_percent;

    uint8_t digital_input_state;
    uint8_t alarm_state;
    uint8_t sensor_valid_flags;
    uint8_t comm_status;
} device_data_snapshot_t;
```

UI 和 Modbus 读取的是快照，而不是直接读硬件。

快照应由后台数据服务定期更新，或在关键数据变化时更新。

## 前端请求机制

如果 UI 或 Modbus 需要修改设备状态，不应直接操作底层模块，而应提交请求。

例如：

```text
设置时间
修改报警阈值
修改 Modbus 地址
修改 IP 地址
清除报警
清除历史记录
查询指定历史记录
恢复默认参数
```

这些操作应通过统一请求接口进入后台数据服务层。

推荐方式：

```text
Message Queue
Mailbox
Event
Semaphore
Mutex-protected API
```

不要让多个前端同时直接写同一份参数结构体或 Flash 存储区。

## Modbus 架构要求

Modbus RTU 和 Modbus TCP 都属于前端协议适配层。

要求：

- RTU 和 TCP 共用同一套寄存器映射。
- 寄存器表只负责把数据服务层的数据映射为 Modbus 寄存器。
- Modbus 不直接读取 ADC。
- Modbus 不直接写 Flash。
- Modbus 不直接操作 LCD。
- Modbus 写参数时，应提交给后台数据服务处理。
- Modbus 查询历史记录时，应通过 record service 获取。

推荐结构：

```text
modbus_rtu_thread
modbus_tcp_thread
  ↓
modbus_map.c
  ↓
data_service / param_service / record_service
```

## UI 架构要求

UI 属于前端显示层。

要求：

- UI 不直接采样。
- UI 不直接写 Flash。
- UI 不直接处理 Modbus。
- UI 通过数据快照刷新页面。
- UI 通过请求接口修改参数。
- UI 刷新不能阻塞数据采集和通信。
- UI 页面逻辑和业务数据逻辑要分离。

推荐结构：

```text
ui_thread
  ↓
ui_page_xxx.c
  ↓
data_service_get_snapshot()
  ↓
data_service_request_xxx()
```

## 采样和事件记录架构要求

采样层负责采集数据，但不负责前端展示。

推荐结构：

```text
adc_sample_thread / timer_isr
  ↓
surge_detect_module
  ↓
data_service_thread
  ↓
record_service_thread
```

采样和浪涌检测模块可以上报：

- 原始 ADC 值
- 峰值
- 触发事件
- 触发时间
- 通道状态
- 有效标志

事件记录由 record service 统一处理。

不要在采样中断中直接写 Flash，也不要在采样中断中通知 UI 刷屏。

## 线程划分建议

推荐 RT-Thread 线程划分：

```text
data_service_thread       核心数据服务
adc_sample_thread         周期采样或采样调度
record_service_thread     事件记录和 Flash 存储
ui_thread                 LCD 显示和菜单
modbus_rtu_thread         RS485 Modbus RTU
modbus_tcp_thread         Ethernet Modbus TCP
sensor_thread             NTC / 温湿度等低速传感器
key_thread                按键扫描
```

线程数量可以根据实际资源裁剪，不要为了架构而过度拆分。

## 并发和数据保护要求

多个线程共享数据时必须考虑并发保护。

要求：

- 多线程共享结构体时，使用 mutex、临界区或双缓冲快照。
- 中断和线程共享变量时，必须使用 volatile 或合适的同步机制。
- Flash 写入必须统一入口。
- 参数修改必须统一入口。
- 报警状态必须统一入口。
- 事件记录编号必须统一分配。
- 不要让多个线程同时写同一个数据结构。

## Codex 修改代码时的要求

当 Codex 修改 UI、Modbus、采样、存储、参数相关代码时，必须先判断当前修改属于：

```text
硬件采集层
后台数据服务层
前端交互层
协议映射层
存储层
```

然后遵守对应边界。

如果发现现有代码已经把 UI、Modbus、采样、Flash 写入混在一起，不要一次性大重构。应先说明问题，再做最小修改，必要时提出分阶段重构方案。

# Codex 回复格式

每次完成修改后，按以下格式回复：

```text
改动内容：
- ...

验证结果：
- ...

可能影响：
- ...

待确认问题：
- ...

建议下一步测试：
- ...
```
