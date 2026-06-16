# PCB L476 + SX1280 + BOLT 技术方案与画板计划

本文档面向当前项目：

- MCU：STM32L476，使用你已有的 L476 最小系统。
- Radio：Semtech SX1280 裸片，参考 `DLP-RFS1280 V1.3` 原理图。
- BOLT：参考原 `DPP2_ComBoard_SX1262_LoRa` 里的 BOLT/接口电路。
- 目标 PCB：整体功能和原来的 `L433 + SX1262 ComBoard` 类似，但是 MCU 换成 L476，radio chip 换成 SX1280。

你是第一次画 PCB，所以这里按“先能画对、再考虑优化”的思路写。射频部分不要自由发挥，优先照参考设计抄。

## 0. 总体结论

推荐最终架构：

```text
STM32L476
  |-- SPI1: PA5/PA6/PA7
  |     |-- SX1280 SPI
  |     `-- BOLT SPI
  |
  |-- SX1280 控制脚: NSS / NRESET / BUSY / DIO1
  |
  |-- BOLT 握手脚: IND / ACK / REQ / MODE / APP_IND
  |
  `-- 调试接口: SWD / UART / LED / 测试点

SX1280
  |-- 52 MHz 晶振
  |-- DC-DC 电感和去耦
  |-- RF 匹配网络
  |-- 2.4 GHz 滤波器
  `-- U.FL 外接 2.4 GHz 天线

BOLT
  |-- MSP430FR5969 BOLT MCU
  |-- 26-pin 板对板连接器
  |-- COM_* / APP_* / BOLT_* 信号
  `-- 供电、复位、去耦
```

第一版 PCB 建议：

- 使用 **4 层板**。
- SX1280 使用 **U.FL 外接天线**，先不要做板载陶瓷天线。
- 不放 `SKY13587-378LF` 天线开关，除非你明确要在板载天线和外接天线之间切换。
- radio RF 匹配和滤波网络必须保留。
- 数字接口尽量完全匹配当前代码的引脚定义，减少软件改动。

## 1. 参考文件与各自用途

### 1.1 `DLP-RFS1280.pdf`

用途：SX1280 裸片射频参考设计。

可以直接参考：

- SX1280 裸片引脚连接。
- 52 MHz 晶振连接。
- `DCC_SW / DCC_FB / VDD_IN` 的 DC-DC 电源结构。
- RFIO 后面的匹配网络。
- 2.4 GHz 滤波器 `2450LP14B100T`。
- U.FL 外接天线连接方式。

不要盲目照抄：

- `SKY13587-378LF` 天线开关，如果你只用 U.FL 外接天线就不需要。
- `NC7SZ04L6X` 反相器，如果不用天线开关就不需要。
- `CHIP ANT` 板载陶瓷天线，如果你第一版只用外接天线就不需要。

### 1.2 `DPP2_ComBoard_SX1262_LoRa-1.pdf`

用途：原来的 ComBoard 整体结构和 BOLT 接口参考。

可以参考：

- BOLT MCU 和 26-pin 连接器。
- `COM_*`、`APP_*`、`BOLT_*` 信号命名。
- 原板的模块划分方式。
- 4 层板思路。
- 调试接口、LED、测试点布局习惯。

不要照抄：

- SX1262 radio 电路不能直接用于 SX1280。
- 868 MHz RF 匹配网络不能用于 2.4 GHz SX1280。
- SX1262 的 32 MHz 晶振不能用于 SX1280；SX1280 用 52 MHz。
- SX1262 的 RF switch、匹配电感电容值不能照搬。原 SX1262 板上用 RF switch，主要是因为 SX1262 的 TX/RX 射频路径和天线网络需要切换；SX1280 是单个 `RFIO` 收发共用口，如果只接一个 U.FL 外接天线，不需要 TX/RX RF switch。

### 1.3 当前代码项目

用途：决定 MCU 和外设实际引脚。

当前关键引脚来自 `Inc/main.h` 和 `Src/main.c`：

| 功能 | 当前代码信号名 | STM32L476 引脚 |
|---|---|---|
| Radio SPI SCK | `RADIO_SCK` / `BOLT_SCK` | PA5 |
| Radio SPI MISO | `RADIO_MISO` / `BOLT_MISO` | PA6 |
| Radio SPI MOSI | `RADIO_MOSI` / `BOLT_MOSI` | PA7 |
| SX1280 NSS | `RADIO_NSS` | PA8 |
| SX1280 NRESET | `RADIO_NRESET` | PA0 |
| SX1280 BUSY | `RADIO_BUSY` | PB3 |
| SX1280 DIO1 wakeup | `RADIO_DIO1_WAKEUP` | PB4 |
| SX1280 DIO1 timer/debug copy | `RADIO_DIO1` | PB11 |
| Radio antenna select | `RADIO_ANTSEL` | PA9 |
| BOLT IND | `BOLT_IND` | PA0 in old code naming conflict? see note below |
| BOLT ACK | `BOLT_ACK` | PB0 |
| BOLT REQ | `BOLT_REQ` | PB1 |
| BOLT MODE | `BOLT_MODE` | PB2 |
| APP IND | `APP_IND` | PA4 |
| COM TREQ | `COM_TREQ` | PA3 |
| COM PROG2 | `COM_PROG2` | PA12 |
| COM PROG | `COM_PROG` | PC4 |
| COM GPIO2 | `COM_GPIO2` | PA11 |
| COM GPIO1 | `COM_GPIO1` | PB5 |

注意：`RADIO_NRESET` 和 `BOLT_IND` 在当前 `Inc/main.h` 都出现了 PA0 的定义风险。你最终 PCB 不能让一个 MCU 引脚同时承担 radio reset 和 BOLT indication 两个不同功能。画板前必须重新确认 CubeMX/代码引脚表，并修正冲突。

## 2. 当前网表必须先检查的问题

你提供的 `Netlist_Schematic1_2026-06-13.tel` 显示出几个必须在嘉立创 EDA 中检查的问题：

### 2.1 Radio 数字信号可能没有连到 MCU

网表中出现：

```text
MISO_TX ; U5.16
MOSI_RX ; U5.17
SCK_RTSN ; U5.18
NSS_CTS ; U5.19
BUSY ; U5.7
DIO1 ; U5.8
DIO2 ; U5.9
DIO3 ; U5.10
NRESET ; U5.3
```

这些网络看起来只挂在 SX1280 `U5` 侧，没有看到对应 MCU `U1` 引脚。也就是说，原理图上可能只是画了网络标签，但标签名没有和 MCU 页面的网络标签一致。

必须修成：

| SX1280 信号 | 应连接到 MCU |
|---|---|
| `NSS_CTS` | PA8 `RADIO_NSS` |
| `SCK_RTSN` | PA5 `SPI1_SCK` |
| `MOSI_RX` | PA7 `SPI1_MOSI` |
| `MISO_TX` | PA6 `SPI1_MISO` |
| `BUSY` | PB3 `RADIO_BUSY` |
| `DIO1` | PB4 `RADIO_DIO1_WAKEUP`，建议同时预留到 PB11 或测试点 |
| `NRESET` | 一个独立 GPIO 输出，例如 PA0，但不能和 BOLT_IND 冲突 |
| `DIO2/DIO3` | 可接 GPIO/测试点，至少预留测试点 |

### 2.2 `3V3` 和 `VDD_3V3` 可能是两个不同电源网

网表中 MCU 使用 `VDD_3V3`，SX1280 使用 `3V3`。如果你在嘉立创 EDA 里用了不同的电源符号，它们可能不会自动相连。

必须统一：

```text
全板 3.3V 电源网只用一个名字，建议统一为 3V3。
```

如果保留不同名字，必须明确用 net-tie 或电源端口连接。

### 2.3 BOLT 和 Radio 共用 SPI1

当前代码里：

```text
PA5 = BOLT_SCK  = RADIO_SCK
PA6 = BOLT_MISO = RADIO_MISO
PA7 = BOLT_MOSI = RADIO_MOSI
```

这可以接受，因为 SPI 总线可以多个从设备共用。

但是必须保证：

- SX1280 有自己的 `NSS`。
- BOLT 通过 `REQ/ACK/MODE` 握手，不是简单 NSS。
- 没有两个设备同时驱动 MISO。
- BOLT 未被请求时，其 SPI 输出必须为高阻或不会抢总线；这要参考原 BOLT 设计。

## 3. MCU 模块方案

### 3.1 MCU 最小系统

你已经有 L476 最小系统，PCB 上至少包含：

- STM32L476 LQFP64。
- 3.3V 供电。
- 每个 VDD 引脚旁边一个 100 nF 去耦电容。
- 每组电源附近加 1 uF 或 4.7 uF/10 uF 储能电容。
- VDDA/VREF+ 供电，建议用磁珠/0R 或小电感从 3V3 分出来，再加 100 nF + 1 uF/10 uF。
- VSS/VSSA 全部接 GND。
- BOOT0 下拉，常用 10 kΩ 到 GND。
- NRST 上拉，常用 10 kΩ 到 3V3，旁边可以放 100 nF 到 GND。
- SWD 接口：SWDIO、SWCLK、NRST、3V3、GND。
- UART 调试口：USART2 PA2/PA3 如果你还要串口日志。
- 8 MHz HSE 和 32.768 kHz LSE 如果你的最小系统已经用了，继续保留。

### 3.2 MCU 与 SX1280 引脚连接

建议第一版尽量不改代码，引脚按当前项目来：

| MCU | SX1280 | 说明 |
|---|---|---|
| PA5 | `SCK_RTSN` | SPI1 SCK |
| PA6 | `MISO_TX` | SPI1 MISO |
| PA7 | `MOSI_RX` | SPI1 MOSI |
| PA8 | `NSS_CTS` | SX1280 片选，空闲高 |
| PB3 | `BUSY` | 输入，SX1280 忙信号 |
| PB4 | `DIO1` | 外部中断，必须接 |
| PA0 或重新选 GPIO | `NRESET` | 输出，空闲高，低复位 |
| PA9 | `ANTSEL` | 如果不用天线开关，可不接或只留测试点 |
| PB11 | `DIO1` copy/test | 当前代码有 `RADIO_DIO1`，建议用 0R 或焊盘选择是否和 DIO1 相连 |

强烈建议：

- `DIO1` 至少接到 PB4。
- `DIO1` 到 PB11 可以通过 0R 电阻或焊盘跳线连接，便于兼容当前代码里“PB4-PB11 jumper”的历史写法。
- `NRESET` 不要和 BOLT_IND 共用 PA0；如果代码和原理图冲突，优先修改代码或重新分配一个空闲 GPIO。

### 3.3 MCU 与 BOLT 引脚连接

BOLT 参考原 DPP2 ComBoard 的接口。当前代码的 BOLT 驱动逻辑是：

- `REQ`：MCU 输出，请求访问 BOLT。
- `ACK`：BOLT 输出，表示接受请求/传输期间有效。
- `MODE`：MCU 输出，0 = READ，1 = WRITE。
- `IND`：BOLT 输出，表示有数据可读。
- `APP_IND`：应用侧输入/状态，表示输出队列有数据。
- SPI：共用 SPI1 的 SCK/MISO/MOSI。

建议信号：

| MCU 信号 | 方向 | 用途 |
|---|---|---|
| `BOLT_REQ` PB1 | MCU -> BOLT | 请求读/写 |
| `BOLT_MODE` PB2 | MCU -> BOLT | 读写模式 |
| `BOLT_ACK` PB0 | BOLT -> MCU | 请求确认/传输有效 |
| `BOLT_IND` | BOLT -> MCU | BOLT 有数据给 MCU |
| `APP_IND` PA4 | BOLT -> MCU 或应用状态 | 输出队列状态 |
| PA5/PA6/PA7 | 双向 SPI | 和 SX1280 共用 SPI1 |

新手注意：

- 输入脚不要悬空；如果原参考图有外部上下拉就照抄。
- 握手线最好预留测试点，因为 BOLT 问题不用示波器很难定位。
- SPI 线也预留测试点，至少 SCK/MOSI/MISO/GND。

### 3.4 MCU 调试和可制造性

必须放：

- SWD 5 针或 4 针接口。
- NRST 测试点。
- 3V3/GND 测试点。
- UART TX/RX 测试点或排针。

建议放：

- 一个电源 LED，串 1 kΩ 到 3.3V 或 GPIO。
- 两个调试 LED，连接到当前代码使用的 `LED_GREEN`、`LED_RED` 或 COM 指示脚。
- 关键 GPIO 旁边丝印网络名。

## 4. RADIO CHIP 模块方案

### 4.1 SX1280 必须连接的引脚

SX1280 裸片是 QFN-24，嘉立创符号可能多一个 `25 GND`，这个是底部裸露焊盘 EP。

必须连接：

| SX1280 引脚 | 连接 |
|---|---|
| `VBAT` | 3V3 |
| `VBAT_IO` | 3V3 |
| `VDD_IN` | 接 `DCC_FB` 节点 |
| `DCC_FB` | 接 `VDD_IN`，并通过 15 uH 到 `DCC_SW` |
| `DCC_SW` | 通过 15 uH 到 `DCC_FB/VDD_IN` 节点 |
| `VR_PA` | 去耦到 GND |
| `XTA/XTB` | 52 MHz 晶振 |
| `RFIO` | RF 匹配网络 |
| `NRESET/BUSY/DIO1/SPI` | 接 MCU |
| 所有 GND | 接 GND |
| `EP/GND pin25` | 接 GND，打过孔到地层 |

### 4.2 SX1280 电源和去耦

照 DLP/Semtech 参考设计：

- `VBAT`：3V3，旁边 100 nF。
- `VBAT_IO`：3V3，旁边 100 nF。
- `VR_PA`：按参考图放 10 nF 或 0.01 uF 到 GND。
- `VDD_IN/DCC_FB`：和 `DCC_SW` 之间放 15 uH 电感。
- `DCC_FB/VDD_IN` 节点放 470 nF 到 GND。
- `DCC_SW` 旁放 100 nF 到 GND。

布局要求：

- 电容必须靠近对应引脚。
- 15 uH 电感靠近 SX1280。
- DCDC 环路短，不要绕远。
- GND 过孔多打，尤其是芯片底部 EP。

### 4.3 52 MHz 晶振

推荐：

- 52 MHz。
- 4-pin SMD2016。
- 负载电容 CL = 10 pF 优先。
- 精度 ±10 ppm 或更好。
- 参考器件：`XC21M4-52.000-F10NNHPL` 这类 52 MHz、10 pF、±10 ppm 的无源晶振。

连接：

```text
Y1 pin 1 -> SX1280 XTB 或 XTA
Y1 pin 3 -> SX1280 XTA 或 XTB
Y1 pin 2 -> GND
Y1 pin 4 -> GND
```

注意：

- 1/3 两脚接振荡信号，方向通常不敏感。
- 2/4 是外壳地，接 GND。
- 第一版不要随便加外部负载电容，DLP 图里没有额外晶振负载电容，SX1280 内部可配置。
- 晶振离 SX1280 越近越好，走线短、对称，下面尽量保持干净地。

### 4.4 RF 匹配和滤波

如果使用 U.FL 外接天线，建议保留：

```text
SX1280 RFIO
 -> C6 0.8pF 到 GND / L3 3nH 串联
 -> L1 2.5nH 串联 + C12 0.5pF 并联/跨接
 -> C5 1.2pF 到 GND
 -> C2 1.2pF 到 GND
 -> C11 100pF 串联
 -> F1 2450LP14B100T
 -> C33 100pF 串联
 -> U.FL 中心脚
```

器件建议：

- RF 小电容使用 C0G/NP0。
- RF 小电感使用高 Q、适合 2.4 GHz 的器件。
- 封装按参考图用 0402/0603，不要随意放大。
- `F1 2450LP14B100T` 是 2.4 GHz RF 滤波器，建议保留。

不要做：

- 不要把 `RFIO` 直接接 U.FL。
- 不要省 RF 匹配网络。
- 不要把 868/915 MHz 的 SX1262 匹配网络搬过来。
- 不要随意改电感电容值。

### 4.5 天线选择

第一版推荐：

```text
只用 U.FL / IPEX 外接 2.4 GHz 天线。
```

保留：

- U.FL 座。
- U.FL 周围 GND。
- U.FL 到滤波器的 50 Ω 射频线。

删除：

- `SKY13587-378LF` 天线开关。
- `NC7SZ04L6X` 反相器。
- `CHIP ANT` 板载天线。
- `ANTSEL` 控制线。
- 板载天线匹配器件。

如果你未来想同时支持板载天线和外接天线，再按 DLP 图把 `SKY13587-378LF` 加回来。第一版不建议这么复杂。

注意：原 SX1262 ComBoard 上的 RF switch 不能作为“我的 SX1280 也必须用”的依据。SX1280 只有在你要做“双天线切换”或外接 PA/LNA 前端时，才需要额外 RF switch/前端控制；单个 U.FL 天线方案不需要。

### 4.6 RF PCB 布局

强烈建议 4 层板：

```text
L1 Top: 元件 + RF 线 + 关键短线
L2 GND: 完整地平面，不切割
L3 Power: 3V3 等电源
L4 Bottom: 普通信号
```

关键规则：

- SX1280、RF 匹配、滤波器、U.FL 放在同一侧，距离尽量短。
- RFIO 到 U.FL 的线保持 50 Ω。
- RF 线下面必须是完整 GND。
- RF 线旁边打地过孔围栏。
- U.FL 放板边。
- U.FL 外壳脚就近接地，多打过孔。
- 晶振不要贴着 RF 输出走线。
- SPI 高速线不要从 RF 匹配网络下面穿过。
- 电源开关、电感、大电流线不要靠近 RFIO 和晶振。

## 5. BOLT 电路模块方案

### 5.1 BOLT 是什么

BOLT 不是普通 SPI Flash。它是原 ETH DPP2 设计里的通信/缓存接口，使用一个 MSP430FR5969 做中间层，并通过握手线和 SPI 与主 MCU 交互。

当前代码里的 BOLT 驱动特点：

- `REQ` 拉高后等待 `ACK`。
- `MODE=0` 表示读，`MODE=1` 表示写。
- 数据通过 SPI 一字节一字节读写。
- `IND` 表示 BOLT 有数据可读。
- `APP_IND` 表示输出队列状态。

### 5.2 建议照抄原 DPP2 BOLT 页

从 `DPP2_ComBoard_SX1262_LoRa-1.pdf` 的 BOLT 页保留：

- `MSP430FR5969 rev. H`。
- MSP430 的 VCC/GND/AVCC/AVSS 供电。
- 每组电源旁边 100 nF + 10 uF 去耦。
- BOLT reset 电路：47 kΩ 上拉/下拉结构和 1.5 nF 电容按原图核对后照抄。
- 26-pin Molex 板对板连接器 `J400`。
- `COM_*`、`APP_*`、`BOLT_*` 网络命名。

如果你只是想兼容原 ComBoard 外形/接口，BOLT 页尽量不要改。BOLT 比 SX1280 更像“系统接口协议”，随便改容易导致上层板对不上。

### 5.3 BOLT 连接器信号

原设计 26-pin 连接器大致包含：

| 信号 | 说明 |
|---|---|
| `COM_VCC` / `BOLT_VCC` | 供电 |
| `BOLT_RXD/TXD/RST/PROG` | BOLT MCU 编程/串口 |
| `BOLT_GPIO1/GPIO2` | BOLT 扩展 GPIO |
| `APP_ACK/REQ/MODE/IND` | 应用侧握手 |
| `APP_SCK/MISO/MOSI` | 应用侧 SPI |
| `COM_IND/REQ/ACK/MODE` | 通信侧握手 |
| `COM_SCK/MISO/MOSI` | 通信侧 SPI |
| `COM_TREQ` | 时间请求 |
| `COM_GPIO1/COM_GPIO2` | 扩展/指示 |
| `COM_PROG/COM_PROG2` | 调试/扩展 |
| `COM_RXD/TXD/RST` | 通信 MCU UART/reset |
| `GND` | 地 |

你当前板子作为通信板，应重点保证 `COM_*` 侧接到 L476。

### 5.4 BOLT 与 L476 的连接检查

必须保证：

```text
COM_SCK  -> L476 PA5 / SPI1_SCK
COM_MISO -> L476 PA6 / SPI1_MISO
COM_MOSI -> L476 PA7 / SPI1_MOSI
COM_REQ  -> L476 BOLT_REQ
COM_ACK  -> L476 BOLT_ACK
COM_MODE -> L476 BOLT_MODE
COM_IND  -> L476 BOLT_IND
COM_TREQ -> L476 COM_TREQ
```

`COM_GPIO1/2`、`COM_PROG/2` 按当前代码或原板需求连接。

注意：

- 不要让 BOLT 的 `COM_IND` 和 SX1280 的 `NRESET` 同时占用 PA0。
- 如果必须兼容原代码，要修改引脚表并重新生成/调整 `Inc/main.h`。
- 如果必须兼容原 ComBoard 接口，要优先保持连接器 pinout 不变，再改 MCU 侧 GPIO。

### 5.5 BOLT PCB 布局

BOLT 不是射频部分，布局要求没有 SX1280 那么敏感，但要注意：

- 26-pin 连接器位置要与原 ComBoard 机械结构一致。
- MSP430 去耦电容靠近电源脚。
- SPI 线短一些，避免绕过 RF 区。
- COM/BOLT/APP 信号线上留测试点。
- 连接器周围丝印标 pin1。
- 连接器机械孔/定位/高度按原件数据手册确认。

## 6. 推荐原理图页面结构

你的嘉立创工程可以这样分页面：

```text
Schematic1
  1. P1 / Power / Connector
  2. MCU_STM32L476
  3. Radio_SX1280
  4. BOLT
  5. Debug_Testpoints
```

每页只做一类事情，不要把所有东西塞一页。

网络命名建议：

| 类型 | 建议命名 |
|---|---|
| 全板 3.3V | `3V3` |
| 地 | `GND` |
| Radio SPI | `RADIO_SCK/RADIO_MISO/RADIO_MOSI/RADIO_NSS` |
| SX1280 原脚名 | `SCK_RTSN/MISO_TX/MOSI_RX/NSS_CTS` 可只在 radio 页局部标注 |
| BOLT SPI | 如果共 SPI，用 `SPI1_SCK/SPI1_MISO/SPI1_MOSI` 更清楚 |
| BOLT 握手 | `BOLT_REQ/BOLT_ACK/BOLT_MODE/BOLT_IND/APP_IND` |

建议做法：

- MCU 页用代码信号名。
- Radio 页可以用芯片原脚名，但必须通过网络标签明确连接到 MCU 页。
- 最终导出网表后检查每个关键网络至少有两个端点。

## 7. PCB 绘制顺序

### Step 1：先修原理图

检查并修正：

- MCU 电源。
- radio 电源。
- BOLT 电源。
- `3V3/VDD_3V3` 是否统一。
- SX1280 数字线是否连到 MCU。
- BOLT 信号是否连到 MCU。
- PA0 冲突是否解决。
- SWD/UART/测试点是否放好。

### Step 2：跑 ERC

在嘉立创 EDA 里跑电气规则检查。

不能忽略：

- 电源未连接。
- 输入悬空。
- 同名网络没有连上。
- 多个输出短接。
- GND 未连接。

### Step 3：封装核对

逐个核对：

- STM32L476 LQFP64 封装和具体型号一致。
- SX1280 QFN-24 EP 封装方向、pin1、裸露焊盘尺寸正确。
- 52 MHz 晶振封装 SMD2016-4P 引脚 1/3 是晶振脚，2/4 是 GND。
- U.FL 封装中心脚和外壳地正确。
- RF 滤波器 `2450LP14B100T` pinout 正确。
- 26-pin 连接器方向和 pin1 正确。

### Step 4：先摆器件

摆放优先级：

1. 机械连接器、U.FL、板对板连接器。
2. SX1280 + RF 匹配 + U.FL。
3. MCU。
4. BOLT MCU + BOLT 连接器。
5. 电源、调试接口、LED、测试点。

射频部分必须紧凑，不要等最后才摆。

### Step 5：先走 RF

先走：

```text
SX1280 RFIO -> 匹配网络 -> F1 -> U.FL
```

然后立刻打地过孔围栏。

### Step 6：走电源和地

- L2 做完整 GND。
- 3V3 用足够宽的线或铺铜。
- SX1280 电源去耦就近。
- MCU 去耦就近。
- BOLT 去耦就近。

### Step 7：走 SPI 和 GPIO

- SPI 尽量短。
- SCK 不要绕 RF 区。
- BUSY/DIO1/NRESET 远离 RF 匹配网络。
- 关键线上预留测试点。

### Step 8：DRC + 网表复查

跑 DRC 后，再回头查网表：

- `3V3` 是否覆盖 MCU/SX1280/BOLT。
- 每个 GND pin 是否都接地。
- SX1280 `pin25/EP` 是否接地。
- `RFIO` 是否只连 RF 匹配网络，不碰数字线。
- U.FL 外壳是否接 GND。

## 8. 新手最容易犯的错误清单

画完板前逐条打勾：

- [ ] `3V3` 和 `VDD_3V3` 没有被画成两个孤立电源。
- [ ] SX1280 的 `NSS/SCK/MISO/MOSI/BUSY/DIO1/NRESET` 真的连到了 MCU。
- [ ] `RADIO_NRESET` 没有和 `BOLT_IND` 共用同一个 PA0。
- [ ] SX1280 所有 GND 和底部裸露焊盘都接 GND。
- [ ] 52 MHz 晶振 1/3 接 XTA/XTB，2/4 接 GND。
- [ ] 没有把 52 MHz 晶振错接成四个信号脚。
- [ ] RFIO 没有直接接天线。
- [ ] 2.4 GHz RF 匹配网络没有省。
- [ ] 没有照抄 SX1262 的 868 MHz RF 匹配网络。
- [ ] U.FL 是 2.4 GHz 天线接口，外壳接地。
- [ ] `SKY13587-378LF` 如果不用板载天线就删除，不要半接。
- [ ] SPI1 共用时，确认 BOLT 和 SX1280 不会同时驱动 MISO。
- [ ] SWD 接口方向正确。
- [ ] BOOT0 默认下拉。
- [ ] NRST 有上拉/复位电路。
- [ ] 每个 IC 电源脚旁边都有 100 nF。
- [ ] 连接器 pin1 方向有丝印。
- [ ] 关键测试点有丝印网络名。

## 9. 建议的第一版取舍

为了第一次 PCB 成功率更高，建议第一版：

保留：

- STM32L476 最小系统。
- SX1280 裸片。
- 52 MHz 晶振。
- SX1280 DCDC 电源结构。
- SX1280 RF 匹配网络。
- `2450LP14B100T` 2.4 GHz 滤波器。
- U.FL 外接天线。
- BOLT 页核心电路。
- SWD、UART、测试点。

暂时删除或不装：

- 板载陶瓷天线。
- `SKY13587-378LF` 天线开关。
- `NC7SZ04L6X` 反相器。
- 没用到的 antenna select 电路。
- 复杂的 RF 屏蔽罩，第一版可以预留焊盘但不一定装。

## 10. 第一版上电调试顺序

焊好板后不要直接跑完整 LWB。按这个顺序：

### 10.1 空板检查

- 万用表量 3V3 到 GND 是否短路。
- 量 SX1280 `VBAT/VBAT_IO` 到 GND 是否短路。
- 量 MCU VDD 到 GND 是否短路。

### 10.2 只上电

- 上 3.3V，限流。
- 看电流是否异常。
- 量 MCU 3V3。
- 量 SX1280 3V3。
- 量 BOLT 3V3。

### 10.3 MCU 下载

- SWD 能识别 L476。
- 下载最简单 blink 程序。
- UART 日志能输出。

### 10.4 SX1280 SPI

- 先只测 SPI 读写寄存器。
- 检查 `BUSY` 是否正常变化。
- 检查 `NRESET` 是否能复位芯片。
- 检查 `DIO1` 中断是否能进 EXTI。

### 10.5 Radio 发射测试

- 先低功率。
- 接 2.4 GHz 天线或衰减器。
- 不接天线不要长时间发射。
- 再跑 LoRa/GFSK/FLRC。

### 10.6 BOLT 测试

- 测 `REQ/MODE/ACK/IND`。
- 测 SPI 写入/读取。
- 再跑完整应用。

## 11. 下一步建议

你现在最应该做的不是马上布线，而是先完成原理图净表检查：

1. 统一全板 3.3V 网络名。
2. 修正 SX1280 数字信号到 MCU 的连接。
3. 解决 PA0 上 `RADIO_NRESET` 和 `BOLT_IND` 的冲突。
4. 决定第一版是否只用 U.FL 外接天线。
5. 导出新网表，确认每个关键网络都有 MCU 和外设两个端点。

这些检查通过后，再开始 PCB layout。射频板最大的问题通常不是“不会布线”，而是原理图阶段就有一个网络名没连上，最后板子做出来完全没法调。
