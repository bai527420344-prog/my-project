# 周报

日期：2026-05-28

## 本周重点

- 项目：完成当前 `L476 + SX1280` 迁移状态总结。
- 射频：整理并验证 `SX1262 -> SX1280` 迁移结果，覆盖 LoRa、GFSK 和 FLRC 调制方式。
- 硬件：从已经验证的 `NUCLEO-L476RG + DLP-RFS1280` 原型平台，准备过渡到自研 `L476 + SX1280` comboard PCB。
- 文档：更新中期答辩材料、项目结构说明和运行逻辑说明。

## 硬件进展

- 当前已经验证的硬件平台为 `NUCLEO-L476RG + DLP-RFS1280`。
- SX1280 模块通过 `SPI1` 与 MCU 连接：
  - `PA5`：SCK
  - `PA6`：MISO
  - `PA7`：MOSI
  - `PA8`：NSS
- 关键控制和中断引脚如下：
  - `PA0`：SX1280 `NRESET`
  - `PB3`：SX1280 `BUSY`
  - `PB4`：SX1280 `DIO1`，连接到 `EXTI4`
  - `PB11`：SX1280 `DIO1`，连接到 `TIM2_CH4` 输入捕获
  - `PA9`：`ANTSEL`
- 在当前开发板原型上，仍然需要 `PB4 -> PB11` 跳线，因为同一个 `DIO1` 信号需要同时提供给：
  - `PB4 / EXTI4`：用于唤醒和射频事件处理
  - `PB11 / TIM2_CH4`：用于高精度时间戳捕获
- `PB3 / SWO` 引脚冲突已经在固件中处理：在 GPIO 初始化前清除 debug trace 配置，避免 SWO 占用 `PB3`。
- DLP-RFS1280 模块目前仍存在一个实际限制：模块侧 `NRESET` 不能被 MCU 稳定拉低。正常通信可以工作，但自研 PCB 阶段需要重新设计 reset 路径。

## 项目进展

- 项目已经从之前的 `L476 + SX1262` 基线迁移到当前 `L476 + SX1280` 仓库。
- 当前仓库处于第三个主要迁移阶段：

```text
L433 + SX1262 原始 comboard
        -> L433 + SX1262 NUCLEO 适配
        -> L476 + SX1262 MCU 迁移
        -> L476 + SX1280 射频迁移  （当前阶段）
        -> 自研 L476 + SX1280 comboard PCB
```

- 主线 R7 代码清理也已经完成，包括删除不再使用的 SX126x 驱动文件。
- LWB/Gloria 协议结构保持不变，主要修改集中在：
  - SX1280 驱动集成
  - radio 常量和调制参数表
  - Gloria 时序参数
  - 板级 GPIO 和 DIO1 时间戳路径
  - FLRC 支持

## 验证状态

以下 SX1280 调制方式已经通过两块实体板子验证。测试方式为 `HOST + NODE`，每 15 秒运行一轮 LWB：

| `GLORIA_INTERFACE_MODULATION` | 调制方式 | 状态 |
|---|---|---|
| `7` | LoRa SF5 | 已验证 |
| `8` | GFSK 125 kbit/s | 已验证 |
| `11` | FLRC 260 kbit/s CR=1/2 | 已验证 |

另外，代码中还保留了以下调制参数表项，但它们不是当前的工作基线：

| `GLORIA_INTERFACE_MODULATION` | 调制方式 | 状态 |
|---|---|---|
| `9` | GFSK 200 kbit/s | 表项已存在，但尚未端到端验证 |
| `10` | GFSK 250 kbit/s | 表项已存在，但不是当前使用的工作模式 |
| `12` | FLRC 650 kbit/s | 表项已存在，但尚未端到端验证 |
| `13` | FLRC 1300 kbit/s | 表项已存在，但尚未端到端验证 |

当前默认配置为：

```c
#define GLORIA_INTERFACE_MODULATION   11
#define GLORIA_INTERFACE_RF_BAND      24   /* 2450 MHz */
#define GLORIA_INTERFACE_POWER        10
#define LWB_SCHED_PERIOD              15
#define LWB_N_TX                      2
#define LWB_NUM_HOPS                  6
```

固件启动时会打印当前调制方式，例如：

```text
INFO task_com: modulation index 11: FLRC 260 kbit/s
```

## 主要技术成果

- 已完成 SX1280 的 SPI 通信、初始化、`BUSY` 握手、`DIO1` 中断处理，以及定时 TX/RX 路径 bring-up。
- LWB/Gloria 已经可以运行在 SX1280 射频栈上，并且没有重写上层 LWB 调度逻辑。
- LoRa、GFSK、FLRC 这些与 SX1280 相关的调制方式都可以通过同一个 `GLORIA_INTERFACE_MODULATION` 配置路径切换。
- 在 LoRa 和 GFSK 基础上新增了 FLRC 支持：
  - 新增 FLRC 枚举和 packet 参数支持
  - 在 radio 配置路径中加入 `MODEM_FLRC` 分支
  - 在 `radio_modulations[]` 中加入 FLRC 表项
  - 在 `gloria_timings[]` 中加入 FLRC 时序参数
  - 增加 `flrc_test` CLI 支持
- 修复了多个从 SX1262 代码迁移到 SX1280 后暴露出的驱动适配问题。它们主要不是 SX1280 芯片本身的 bug，而是两代 radio 在命令编码、寄存器长度、packet 参数和调制模式支持上的差异：
  - RX/TX timeout 编码：
    - 原逻辑：沿用 SX126x 风格，把 `timeout_ms` 左移 6 位；连续接收时使用类似 `0xFFFFFF` 的值；`timeout_ms = 0` 时会直接传 `0`。
    - 问题：SX1280 的 `SetRx/SetTx` timeout 是 3 字节格式：`PeriodBase + Count MSB + Count LSB`。`PeriodBase` 只能是 `0/1/2/3`，原来的左移结果会把非法值写进第 1 个字节。并且 `Count = 0` 在 SX1280 上是 single mode，不是“一直等”。
    - 当前逻辑：新增 `SX1280_FormatRxTxTimeout()`。`timeout_ms = 0` 或 continuous 时写 `0x00FFFF`，表示 `PeriodBase = 15.625 us`、`Count = 0xFFFF`，即连续 RX；普通超时写 `(0x02 << 16) | timeout_ms`，表示 `PeriodBase = 1 ms`、`Count = timeout_ms`。
  - whitening seed 写入长度：
    - 原逻辑：按 SX126x 的 9-bit whitening seed 写 2 字节到 `0x09C5` 和 `0x09C6`。
    - 问题：SX1280 的 whitening seed 只需要写 1 字节到 `REG_LR_WHITSEEDBASEADDR = 0x09C5`；在当前寄存器布局中，`0x09C6` 是 CRC polynomial 的 MSB。原逻辑会顺手覆盖 CRC polynomial（发送端和接收端约定好的一个“除数规则”。发送端用它根据 payload 算出 CRC 校验值；接收端收到数据后，也用同一个 polynomial 重新算一遍。如果两边结果不一致，就说明数据可能在传输中出错了。CRC 本质上是用二进制多项式除法来做错误检测。），导致 CRC 引擎配置被破坏。
    - 当前逻辑：`SX1280SetWhiteningSeed(0x01FF)` 保留函数接口，但只写低 8 bit 到 `0x09C5`。
  - GFSK variable-length packet（SX1280 里 GFSK/FLRC 的变长包必须写 0x20，如果沿用 SX1262 的 0x01，芯片会误解 packet 格式，导致接收失败。） 常量：
    - 原逻辑：继承 SX1262 的 variable packet 常量，等价于 `0x01`。
    - 问题：在 SX1280 GFSK/FLRC 的 `PacketParam4 HeaderType` 中，variable-length 应该是 `0x20`，fixed-length 应该是 `0x00`。
    - 当前逻辑：重新定义并使用 `RADIO_PACKET_VARIABLE_LENGTH = 0x20` 和 `RADIO_PACKET_FIXED_LENGTH = 0x00`。
  - LoRa fixed-length packet 常量：
    - 原逻辑：把 `fixLen` 这种 bool 值直接 cast 成 LoRa header type，fixed 时得到 `0x01`。
    - 问题：SX1280 LoRa 的 explicit/variable 是 `0x00`，implicit/fixed 是 `0x80`，`0x01` 不是正确的 fixed-length 配置。
    - 当前逻辑：使用 `LORA_PACKET_VARIABLE_LENGTH = 0x00` 和 `LORA_PACKET_FIXED_LENGTH = 0x80`，不再直接把 bool 当寄存器值。
  - CRC type 映射：
    - 原逻辑：CRC enum 值被直接当成 SX1280 packet 参数写入，但 SX1262 的枚举值和 SX1280 的 packet 参数不完全一致。
    - 问题：SX1280 packet 参数里 CRC 长度需要写成芯片侧编码，例如 2-byte CRC 对应 `0x20`，3-byte CRC 对应 `0x30`；代码内部还需要区分 CCITT polynomial。
    - 当前逻辑：GFSK 开 CRC 时设置为 `RADIO_CRC_2_BYTES_CCIT`，映射成芯片参数 `0x20`，并配置 CCITT polynomial；FLRC 开 CRC 时设置为 `RADIO_CRC_3_BYTES`，芯片参数为 `0x30`。
  - FLRC 3-byte CRC：
    - 原逻辑：FLRC 刚加入时容易沿用 GFSK/LoRa 的 2-byte CRC 或 bool CRC 处理方式。
    - 问题：当前 FLRC baseline 按 SX1280 FLRC packet format 使用 3-byte CRC，不能和 GFSK 的 2-byte CCITT CRC 配置混在一起。
    - 当前逻辑：在 `MODEM_FLRC` 的 RX/TX 配置中设置 `CrcLength = RADIO_CRC_3_BYTES`；关闭 CRC 时设置 `RADIO_CRC_OFF`。
  - FLRC 最大 payload 长度更新路径：
    - 原逻辑：`RadioSetMaxPayloadLength()` 只有 LoRa/GFSK 分支，非 LoRa 路径默认走 GFSK 结构体字段。
    - 问题：`SX1280.PacketParams` 是 union，GFSK 和 FLRC 的 `PayloadLength` 在不同位置；把 FLRC 当 GFSK 改会读错 `HeaderType`，甚至覆盖 FLRC 的 `CrcLength`。
    - 当前逻辑：增加 `PACKET_TYPE_FLRC` 分支，只在 `Flrc.HeaderType == RADIO_PACKET_VARIABLE_LENGTH` 时更新 `Flrc.PayloadLength`。
  - FLRC sync word 写入路径：
    - 原逻辑：共用 GFSK 的 `SX1280SetSyncWord()` 路径，从 `REG_LR_SYNCWORDBASEADDRESS = 0x09CE` 开始写 8 字节。
    - 问题：FLRC 使用 4-byte sync word，应写到 `REG_LR_SYNCWORDBASEADDRESS + 1 = 0x09CF`；同时使用 `SyncWordLength = FLRC_SYNC_WORD_LEN_P32S` 和 `SyncWordMatch = 0x10`，只匹配 SW1。
    - 当前逻辑：GFSK 继续使用原来的 8 字节 sync word 路径；FLRC 在 `MODEM_FLRC` 分支中直接写 `FlrcSyncWord = { 0xDD, 0xA0, 0x96, 0x69 }` 到 `0x09CF`。
  - LWB payload 长度限制：
    - 原逻辑：LoRa 路径可使用较大的 LWB payload，默认按 `80` 字节配置。
    - 问题：在当前两板验证环境下，GFSK/FLRC 链路预算不适合直接使用 LoRa 的大 payload；同时 LWB data packet 长度必须小于 Gloria/radio 支持的最大 payload。
    - 当前逻辑：在 `Inc/app_config.h` 中按调制模式限制 payload：`GLORIA_INTERFACE_MODULATION >= 8` 时 `LWB_MAX_PAYLOAD_LEN = 32`，LoRa 模式保持 `80`。这样仍然可以容纳 DPP 最小消息长度 `18` 字节，并给实际应用 payload 留出约 `14` 字节空间。

当前固件可以较宽松地放入 `STM32L476RG` 的资源限制内：

- Flash 使用量：约 `127.7 KB / 1024 KB`
- RAM 使用量：约 `34.7 KB / 96 KB SRAM1`

## 已知限制

- 多跳传播延迟、8 跳传播时间、同步误差和 packet delivery ratio 尚未在更大节点拓扑中实测。
- 当前验证基于两块实体板子：一个 host 和一个 source node。
- DLP-RFS1280 模块的 reset 行为目前不能完全由 MCU 控制，这一点需要在自研 PCB 设计中修复。
- 当前开发板搭建仍然依赖 `PB4 -> PB11` 跳线，以支持 `DIO1` 的双用途路径。

## 下一步计划

- 从已经验证的开发板原型过渡到自研 PCB 阶段。
- 设计 `STM32L476RG + SX1280` comboard 原理图。
- 保留已经验证过的关键电气选择：
  - `SPI1` 引脚映射
  - `DIO1` 同时连接到 `PB4` 和 `PB11`
  - `BUSY` 使用 `PB3`，并避免 SWO 冲突
  - 可由 MCU 控制的 `NRESET`
  - 8 MHz HSE 和 32.768 kHz LSE
  - SWD 和 UART 调试接口
- 为 `NRESET`、`BUSY`、`DIO1`、`NSS`、`SCK` 和 TX/RX 指示信号增加测试点。
- PCB bring-up 后，重复以下验证流程：
  1. MCU 时钟和 UART 日志
  2. SPI 和 SX1280 status 读取
  3. SX1280 初始化
  4. DIO1 中断和时间戳捕获
  5. LoRa SF5 LWB 验证
  6. GFSK 125k LWB 验证
  7. FLRC 260k LWB 验证

## 总结

项目已经在 `STM32L476RG + SX1280` 平台上验证了 LoRa、GFSK 和 FLRC 三种模式下的 LWB 运行，修复了多个 radio driver 和 packet format 相关问题，并为下一阶段自研 PCB 奠定了技术基础。
