# Modbus 临时寄存器表

本文记录当前后端验证阶段的 Modbus RTU 临时寄存器映射。正式产品寄存器表冻结前，地址仍可调整；但代码变更时必须同步更新本文档。

当前固件尚未对外发行，不保留旧开发期报警闭合记录格式兼容。报警历史采用事件日志模型，`event_action` 是正式字段，不再使用旧的 `end_timestamp_ms` 或结束时间字段。

## 基本约定

- 当前从站地址由参数 `modbus_addr` 决定，当前测试值常用 `2`。
- 当前 RTU 串口为 COM16，默认 `9600 8N1`。
- 本文地址使用“业务寄存器号”，与 FreeModbus 回调中的地址一致；部分主站工具使用 0 基协议地址，填写时需要减 1。
- Modbus TCP 使用同一套 Holding / Input 寄存器映射。当前 W5500 版本只使用 `socket0`，第一版只支持 1 路 Modbus TCP client 并发连接。
- Modbus TCP 对空闲 TCP 连接设置 5 秒超时。客户端连上后如果长期不发送完整 Modbus TCP 请求，固件会主动关闭 socket0 并重新进入监听，避免单 socket 被异常客户端长期占用。
- Holding Register 为运行参数、异步命令和查询控制。
- Input Register 为实时快照、报警状态和查询结果，只读。
- Modbus 回调不得执行 EEPROM、Flash、I2C、ADC 采样等慢操作，只读写 RAM 快照或向后端线程投递请求。

## Holding Register 1..99：参数区

参数区由 `param_service` 显式声明 Holding 地址，不再依赖 `param_service_id_t + 1`。当前已有基础参数仍使用 `1..17`，以太网 / Modbus TCP 参数使用 `50..63`。

| 地址 | 参数 ID | 说明 |
| --- | --- | --- |
| 1 | `PARAM_SERVICE_ID_MODBUS_ADDR` | Modbus RTU 从站地址 |
| 2 | `PARAM_SERVICE_ID_MODBUS_BAUDRATE` | Modbus RTU 波特率 |
| 3 | `PARAM_SERVICE_ID_MODBUS_DATA_FORMAT` | Modbus RTU 数据格式：`0=8N1`，`1=8E1`，`2=8O1`，`3=8N2`，`4=8E2`，`5=8O2` |
| 4 | `PARAM_SERVICE_ID_ALARM_PEAK_MILLI_KV` | 浪涌峰值报警阈值，寄存器单位为 0.1 kV |
| 5 | `PARAM_SERVICE_ID_DISPLAY_BRIGHTNESS_PERCENT` | 显示亮度百分比 |
| 6 | `PARAM_SERVICE_ID_ENV_TEMP_OFFSET_MC` | 环境温度校准偏移，按 int16 解释 |
| 7 | `PARAM_SERVICE_ID_ENV_HUMI_OFFSET_MPERMIL` | 环境湿度校准偏移，按 int16 解释 |
| 8 | `PARAM_SERVICE_ID_TI_ALARM_ENABLE_MASK` | TI1..TI5 报警使能 mask |
| 9 | `PARAM_SERVICE_ID_TI_ALARM_OPEN_LEVEL_MASK` | TI1..TI5 open 电平 mask，默认 `0` 表示低电平为 open |
| 10 | `PARAM_SERVICE_ID_NTC_SENSOR_ENABLE` | NTC 传感器使能，`0`=禁用，`1`=使能 |
| 11 | `PARAM_SERVICE_ID_NTC_HIGH_ALARM_ENABLE` | NTC 高温报警使能，默认 `0` |
| 12 | `PARAM_SERVICE_ID_NTC_HIGH_ALARM_THRESHOLD_MC` | NTC 高温报警阈值，寄存器单位为 0.01 摄氏度 |
| 13 | `PARAM_SERVICE_ID_NTC_LOW_ALARM_ENABLE` | NTC 低温报警使能，默认 `0` |
| 14 | `PARAM_SERVICE_ID_NTC_LOW_ALARM_THRESHOLD_MC` | NTC 低温报警阈值，寄存器单位为 0.01 摄氏度 |
| 15 | `PARAM_SERVICE_ID_TI_ALARM_DEBOUNCE_MS` | TI 报警消抖时间，单位 ms |
| 16 | `PARAM_SERVICE_ID_NTC_ALARM_DEBOUNCE_MS` | NTC 报警发生和恢复消抖时间，单位 ms |
| 17 | `PARAM_SERVICE_ID_SURGE_ALARM_ENABLE` | 浪涌报警使能，默认 `1` |
| 50 | `PARAM_SERVICE_ID_NET_DHCP_ENABLE` | DHCP 使能，`0`=静态 IP，`1`=DHCP；当前默认 `0` |
| 51 | `PARAM_SERVICE_ID_NET_IP_ADDR` | IP 地址第 1 字节，默认 `192` |
| 52 | `PARAM_SERVICE_ID_NET_IP_ADDR` | IP 地址第 2 字节，默认 `168` |
| 53 | `PARAM_SERVICE_ID_NET_IP_ADDR` | IP 地址第 3 字节，默认 `1` |
| 54 | `PARAM_SERVICE_ID_NET_IP_ADDR` | IP 地址第 4 字节，默认 `223` |
| 55 | `PARAM_SERVICE_ID_NET_NETMASK` | 子网掩码第 1 字节，默认 `255` |
| 56 | `PARAM_SERVICE_ID_NET_NETMASK` | 子网掩码第 2 字节，默认 `255` |
| 57 | `PARAM_SERVICE_ID_NET_NETMASK` | 子网掩码第 3 字节，默认 `255` |
| 58 | `PARAM_SERVICE_ID_NET_NETMASK` | 子网掩码第 4 字节，默认 `0` |
| 59 | `PARAM_SERVICE_ID_NET_GATEWAY` | 网关第 1 字节，默认 `192` |
| 60 | `PARAM_SERVICE_ID_NET_GATEWAY` | 网关第 2 字节，默认 `168` |
| 61 | `PARAM_SERVICE_ID_NET_GATEWAY` | 网关第 3 字节，默认 `1` |
| 62 | `PARAM_SERVICE_ID_NET_GATEWAY` | 网关第 4 字节，默认 `1` |
| 63 | `PARAM_SERVICE_ID_MODBUS_TCP_PORT` | Modbus TCP 监听端口，默认 `502` |
| 64 | factory MAC | MAC 地址第 1 字节 |
| 65 | factory MAC | MAC 地址第 2 字节 |
| 66 | factory MAC | MAC 地址第 3 字节 |
| 67 | factory MAC | MAC 地址第 4 字节 |
| 68 | factory MAC | MAC 地址第 5 字节 |
| 69 | factory MAC | MAC 地址第 6 字节 |

IPv4 地址使用 4 个连续 Holding，每个 Holding 只保存一个字节，范围 `0..255`，顺序为点分十进制顺序。例如 `192.168.1.223` 写为 `192, 168, 1, 223`。

写 IPv4 参数时必须一次写完整 4 个连续寄存器，例如写 IP 地址必须一次写 Holding `51..54`。不允许只写某一个字节，避免保存半个新 IP。

factory MAC 使用 6 个连续 Holding，每个 Holding 只保存一个字节，范围 `0..255`，顺序为 MAC 文本顺序。例如 `02:57:50:00:00:01` 写为 `2, 87, 80, 0, 0, 1`。写 MAC 时必须一次写完整 Holding `64..69`，不允许只写某一个字节。写入后由后台 `backend_req` 擦写片内 Flash factory 区，完成状态通过 Holding `102/103` 查询。写入 factory MAC 后不会自动重启 W5500，需要再写 Holding `100=6` 应用网络参数。

## Holding Register 100..107：异步慢命令

该区域用于触发 EEPROM/Flash 或运行期应用操作。Modbus 回调只负责校验和投递请求，实际执行由 `backend_req` 线程完成。

| 地址 | 读含义 | 写含义 |
| --- | --- | --- |
| 100 | 当前/最近命令码 | 写命令码：`1` 保存参数，`2` 从 EEPROM 加载参数，`3` 恢复运行期默认参数，`4` 清除运行期记录，`5` 应用 Modbus RTU 通信参数，`6` 应用网络 / Modbus TCP 参数；MAC 写入完成后读到的最近命令码可能为 `7` |
| 101 | 命令序号低 16 位 | 写 `0xA55A` 解锁破坏性命令 |
| 102 | busy，`1` 执行中，`0` 空闲 | 只读 |
| 103 | 最近命令结果，`0` 表示成功，负数按 int16 解释 | 只读 |
| 104 | 最近完成时间 ms 高 16 位 | 只读 |
| 105 | 最近完成时间 ms 低 16 位 | 只读 |
| 106 | 解锁状态，`1` 已解锁，`0` 未解锁 | 只读 |
| 107 | 保留 | 只读 |

说明：

- 命令 `1`、`2`、`5`、`6` 可直接写地址 `100` 触发。
- 命令 `3`、`4` 需要先向地址 `101` 写 `0xA55A`，再在 5 秒内向地址 `100` 写命令码。
- 命令投递成功只表示请求进入后端队列；执行完成和结果需要轮询地址 `102`、`103`。
- 同一时间只允许一个慢命令处于 busy 状态；busy 未清零前提交新命令会返回忙。
- 命令 `5` 只应用当前 RAM 参数影子到正在运行的 Modbus RTU，不自动保存 EEPROM。
- 命令 `6` 用于应用当前 RAM 中的网络 / Modbus TCP 参数；当前已接入 W5500，会重新配置 IP、子网掩码、网关、MAC 和 Modbus TCP 监听端口，并重启 `socket0` 监听。执行结果通过地址 `102`、`103` 查询。
- 类型 `7` 表示后台正在写入或最近完成写入 factory MAC。上位机不需要向地址 `100` 写 `7`；写 Holding `64..69` 会自动投递该请求。

推荐调整 Modbus RTU 通信参数流程：

```text
1. 写 Holding 1..3 修改 RAM 参数影子。
2. 读 Holding 1..3 确认待应用参数。
3. 需要断电保持时写 Holding 100=1 保存参数，并轮询 102/103。
4. 需要立即生效时写 Holding 100=5 应用参数。
5. 收到 100=5 写响应后，再按新地址、新波特率或新数据格式继续通信。
```

推荐调整网络参数流程：

```text
1. 写 Holding 50..63 修改 RAM 参数影子。
2. 读 Holding 50..63 确认待应用参数。
3. 需要断电保持时写 Holding 100=1 保存参数，并轮询 102/103。
4. 写 Holding 100=6 应用网络参数，并轮询 102/103。
5. 如果通过 Modbus TCP 修改自身 IP 或端口，收到写响应后当前 TCP 连接可能断开；上位机应按新地址重新连接。第一版推荐通过 Modbus RTU 配置网络参数。
```

MAC 地址使用 Holding `64..69` 写入片内 Flash `factory` 区。固件优先使用 factory 区中的 MAC；factory 区未写入有效 MAC 时，开发阶段回退为按 IP 派生的本地管理单播地址，前缀为 `02:57:50`。该 fallback 策略不是量产规则；生产阶段必须通过生产工具写入唯一 MAC，并读回校验。

## Holding Register 120..139：历史记录查询控制

该区域用于查询历史记录窗口。查询只做翻页，不做时间范围查询，不做报警原因过滤。Modbus 回调只负责写入查询参数和投递查询请求，不直接遍历 FlashDB；需要访问 TSDB 的查询由后台 `mb_recq` 线程异步完成。

| 地址 | 读含义 | 写含义 |
| --- | --- | --- |
| 120 | 查询模式：`0` 空闲，`1` 按 RAM 最近 offset 查询，`2` 按 RAM sequence 查询，`3` 按报警历史 offset 查询 | 写查询模式 |
| 121 | 最近 offset，`0` 表示最新一条 | 写 offset |
| 122 | 查询 sequence 高 16 位 | 写 sequence 高 16 位 |
| 123 | 查询 sequence 低 16 位 | 写 sequence 低 16 位 |
| 124 | 触发寄存器，读恒为 0 | 写 `1` 触发一次查询 |
| 125 | busy，`1` 查询线程处理中，`0` 空闲 | 只读 |
| 126 | 最近查询结果，负数按 int16 解释 | 只读 |
| 127 | 查询结果有效，`0` 无效，`1` 有效 | 只读 |
| 128 | 当前 RAM 记录缓存数量 | 只读 |
| 129 | 最新 sequence 高 16 位 | 只读 |
| 130 | 最新 sequence 低 16 位 | 只读 |
| 131 | 最旧 sequence 高 16 位 | 只读 |
| 132 | 最旧 sequence 低 16 位 | 只读 |
| 133 | 查询绑定的记录区 generation 高 16 位 | 只读 |
| 134 | 查询绑定的记录区 generation 低 16 位 | 只读 |
| 135 | 本次查询返回条数 | 只读 |
| 136..139 | 保留 | 只读 |

## Holding Register 140..149：报警记录列表分页控制

该区域用于上位机按页读取报警事件列表。它只查询 `alarm_tsdb` 中的报警事件日志，不查询浪涌详情 `surge_tsdb`，也不做时间范围、报警原因或类型过滤。

| 地址 | 读含义 | 写含义 |
| --- | --- | --- |
| 140 | 翻页命令：`0` 空闲，`1` 加载指定页，`2` 下一页，`3` 上一页，`4` 第一页 | 写翻页命令 |
| 141 | 指定页码，`0` 表示最新第 1 页 | 写指定页码，命令 `1` 有效 |
| 142 | 页大小，当前最大 `4`，写 `0` 表示默认 `4` | 写页大小 |
| 143 | 触发寄存器，读恒为 0 | 写 `1` 触发一次分页查询 |
| 144 | busy，`1` 正在翻页查询，`0` 空闲 | 只读 |
| 145 | done，`1` 已完成至少一次查询 | 只读 |
| 146 | 最近翻页结果，负数按 int16 解释 | 只读 |
| 147 | 当前页码 | 只读 |
| 148 | 当前页返回条数 | 只读 |
| 149 | flags，bit0=has_prev，bit1=has_next | 只读 |

查询完成后读取 Input `160..239` 获取页状态和最多 4 条报警记录。

## Input Register 概览

| 范围 | 含义 |
| --- | --- |
| 1..16 | 实时数据和浪涌调试量 |
| 17..24 | 报警汇总 |
| 25..84 | 当前活跃报警表，6 槽，每槽 10 个寄存器 |
| 85..101 | 最近一条记录摘要 |
| 120..159 | 历史记录查询结果 |
| 160..239 | 报警记录列表分页结果 |
| 240..259 | 网络运行状态 |

## Input Register 160..239：报警记录分页结果

页状态头：

| 地址 | 含义 |
| --- | --- |
| 160 | 当前页快照是否有效 |
| 161 | 当前页码 |
| 162 | 当前页返回条数 |
| 163 | flags，bit0=has_prev，bit1=has_next |
| 164 | 查询 generation 高 16 位 |
| 165 | 查询 generation 低 16 位 |
| 166 | 最近翻页结果，负数按 int16 解释 |
| 167 | 页大小 |
| 168..171 | 保留 |

报警记录槽从 `172` 开始，每条 17 个寄存器，最多 4 条。

| 槽内偏移 | 含义 |
| --- | --- |
| 0 | 槽有效标志 |
| 1 | sequence 高 16 位 |
| 2 | sequence 低 16 位 |
| 3 | record type |
| 4 | alarm_id 高 16 位 |
| 5 | alarm_id 低 16 位 |
| 6 | event_action |
| 7 | alarm_reason |
| 8 | event_timestamp_ms 高 16 位 |
| 9 | event_timestamp_ms 低 16 位 |
| 10 | duration_ms 高 16 位 |
| 11 | duration_ms 低 16 位 |
| 12 | peak_milli_kv，按 int16 解释 |
| 13 | ntc_temp_milli_c / 10，按 int16 解释 |
| 14 | env_temp_milli_c / 10，按 int16 解释 |
| 15 | env_humi_milli_percent / 10 |
| 16 | `digital_input_state << 8 | alarm_state` |

报警记录分页只返回报警事件日志。浪涌详情记录后续单独设计，报警记录中可保留指向浪涌详情的关联字段，但不与报警列表共用同一块 TSDB。

## Input Register 240..259：网络运行状态

该区域只表示当前 W5500 / Modbus TCP 的实际运行状态，不表示 EEPROM 中保存的参数。尤其在 DHCP 使能时，Holding `51..63` 仍然是静态 / fallback 网络参数；DHCP 实际获取到的 IP、子网掩码、网关、DNS 和租约时间应从 Input `240..259` 读取。

| 地址 | 含义 |
| --- | --- |
| 240 | flags：bit0=Modbus TCP 已运行，bit1=当前使用 DHCP 地址，bit2=正在应用网络参数，bit3=网线链路已连接，bit4=100M，bit5=全双工 |
| 241 | W5500 socket0 状态寄存器快照 |
| 242 | 当前运行 IP 第 1 字节 |
| 243 | 当前运行 IP 第 2 字节 |
| 244 | 当前运行 IP 第 3 字节 |
| 245 | 当前运行 IP 第 4 字节 |
| 246 | 当前运行子网掩码第 1 字节 |
| 247 | 当前运行子网掩码第 2 字节 |
| 248 | 当前运行子网掩码第 3 字节 |
| 249 | 当前运行子网掩码第 4 字节 |
| 250 | 当前运行网关第 1 字节 |
| 251 | 当前运行网关第 2 字节 |
| 252 | 当前运行网关第 3 字节 |
| 253 | 当前运行网关第 4 字节 |
| 254 | DHCP DNS 第 1 字节，静态模式或 DHCP 未成功时为 0 |
| 255 | DHCP DNS 第 2 字节，静态模式或 DHCP 未成功时为 0 |
| 256 | DHCP DNS 第 3 字节，静态模式或 DHCP 未成功时为 0 |
| 257 | DHCP DNS 第 4 字节，静态模式或 DHCP 未成功时为 0 |
| 258 | DHCP lease seconds 高 16 位，静态模式或 DHCP 未成功时为 0 |
| 259 | DHCP lease seconds 低 16 位，静态模式或 DHCP 未成功时为 0 |

说明：

- MAC 地址仍通过 Holding `64..69` 读写，不在该状态区重复映射。
- Modbus TCP 监听端口仍通过 Holding `63` 读取；当前版本应用网络参数后 active port 与 Holding `63` 保持一致。
- Input `240..259` 来源于 `app_modbus_tcp` 的 RAM 快照，Modbus 回调不直接访问 W5500 SPI。
