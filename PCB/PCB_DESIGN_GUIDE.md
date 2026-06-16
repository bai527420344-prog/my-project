# PCB 设计指南 — STM32L476RG + SX1280 LWB 项目

本文档从已验证可工作的代码中提炼出 **PCB 设计必须知道的硬件信息**,面向第一次画 PCB 的设计者。已踩过的坑都标了 ⚠️。

> 适用范围: 把当前 `NUCLEO-L476RG + DLP-RFS1280` 飞线原型转为自制 PCB。代码层已完成 R0-R7,3 种 modulation (LoRa SF5 / GFSK 125k / FLRC 260k) 端到端验证通过。

## 目录

1. [核心芯片选择](#1-核心芯片选择)
2. [SPI1 接口](#2-spi1-接口必须保持)
3. [SX1280 GPIO 连接](#3-sx1280-必接-gpio非-spi)
4. [⚠️ PB4 ↔ PB11 同源](#4-️-关键坑pb4--pb11-必须接到同一个-dio1-信号)
5. [⚠️ PB3 BUSY vs SWO 冲突](#5-️-关键坑pb3--busy-vs-swo-冲突)
6. [⚠️⚠️ NRESET 直连](#6-️️-最大坑nreset-必须直接连)
7. [RF 设计](#7-rf-设计最复杂的部分)
8. [电源](#8-电源)
9. [SWD 调试接口](#9-调试--编程接口)
10. [串口](#10-串口用于看-lwb-日志)
11. [时钟源](#11-时钟源代码已配死)
12. [其他 GPIO + Test Point](#12-其他被代码用到的-gpio建议留-test-point)
13. [设计流程](#13-推荐-pcb-设计流程新手向)
14. [必备文档](#14-必备文档)
15. [总体建议](#15-给你的建议顺序)

---

## 1. 核心芯片选择

| 项 | 选用 | 备注 |
|---|---|---|
| MCU | **STM32L476RGTx** (LQFP64) | 80MHz Cortex-M4F + FPU, 1MB Flash, 128KB RAM |
| Radio | **SX1280** (QFN24, 4×4mm) 或 DLP-RFS1280 模组 | 2.4 GHz |
| 模组 vs 裸片 | 推荐**模组**第一版 | 见第 7 节 RF 设计风险 |

---

## 2. SPI1 接口(必须保持)

代码硬编码用 SPI1,跑在 **10 MHz**, CPOL=0/CPHA=0(SX1280 要求)。

| MCU 引脚 | SX1280 信号 | PCB 走线要求 |
|---|---|---|
| **PA5** | SCK | 短、避免与其他高速信号长距离平行 |
| **PA6** | MISO | 与 SCK 等长(不强求,10MHz 不是高速) |
| **PA7** | MOSI | 同上 |
| **PA8** | NSS (片选) | 普通 GPIO 即可 |

走线长度 < 50mm 没问题。10MHz 不算高速,**但** SPI 信号最好走在 GND 平面附近,避免穿过电源平面分割线。

---

## 3. SX1280 必接 GPIO(非 SPI)

| MCU 引脚 | SX1280 信号 | 重要程度 | 原因 |
|---|---|---|---|
| **PA0** | NRESET | ⚠️ **必接,且模组要改硬件** | 见第 6 节 NRESET 坑 |
| **PB3** | BUSY | 必接 | 等待 chip 就绪的握手信号 |
| **PB11** | DIO1 | 必接 | radio 中断 + 时戳捕获(TIM2_CH4) |
| **PB4** | DIO1 WAKEUP | 必接,**与 PB11 同源** | 见第 4 节 |
| **PA9** | ANTSEL | 看天线方案 | 内置天线选择(如果用模组带 TCXO 之类) |

---

## 4. ⚠️ 关键坑:PB4 ↔ PB11 必须接到**同一个** DIO1 信号

代码里这是飞线([README.md:98-107](README.md#L98-L107)),你的 PCB 上必须把 **SX1280 的 DIO1 引脚通过一根 trace 同时接到 PB4 和 PB11**。

**原因**:
- PB11 上跑 `TIM2_CH4` 输入捕获(精确时戳记录 IRQ 到达时刻,LWB 同步需要)
- PB4 上跑 `EXTI4` 中断唤醒(低功耗模式被叫醒)
- 这两个功能必须同时存在,但 STM32 不支持同一外设引脚做两件事

**PCB 上做法**: SX1280_DIO1 → 一根 trace 同时连 MCU_PB4 和 MCU_PB11,就一根 net,不要任何跳线/电阻分隔。

---

## 5. ⚠️ 关键坑:PB3 = BUSY vs SWO 冲突

代码里 [Src/main.c:109-120](Src/main.c#L109-L120) 有专门一段在 boot 时清 `DBGMCU->CR TRACE_IOEN` bit。

**原因**:
- PB3 默认是 JTDO/TRACESWO(调试接口)
- 你要把它当 SX1280 BUSY 用
- ST-LINK 调试器有时会在 reset 前置 TRACE_IOEN,让 PB3 被 trace 占用 → 起不来

**PCB 设计含义**:
- **不要**把 PB3 连到调试接口的 SWO 引脚
- ST-LINK 连接器(见第 9 节)上 SWO pin 留 NC(不接)
- 这样物理上就不可能冲突

---

## 6. ⚠️⚠️ 最大坑:NRESET 必须直接连

### 6.1 问题现象

当前用的 **DLP-RFS1280 模组**内部把 NRESET 拉高到一个很难驱动的状态,STM32 即使 GPIO 输出 0 也压不下去。**SX1280 datasheet 说 NRESET 内部只有 50kΩ 上拉,理论上 STM32 push-pull 输出应该能轻松拉低,但实测做不到**。

**故障分离实验**(已做过):
- 把 DLP-RFS1280 模组拆下来 → MCU PA0 能正常输出 0 (~ 0V)
- 把模组装回去 → MCU PA0 输出 0 时电压依然停在 ~3.3V
- **结论**: 模组内部有额外的上拉电路(或保护二极管/三极管),公开 datasheet 没说

### 6.2 这个坑带来的实际影响

1. **软件无法 reset SX1280**: 标准的 `NRESET 拉低 → 延时 → 拉高` 流程不工作
2. **芯片状态错乱时只能拔 USB**: 比如调试时发了一个不合法命令,芯片卡在某个未定义状态 — 软 reset 救不回来
3. **代码已经加了 workaround**(但不优雅,见下)

### 6.3 代码层的 workaround(供参考,**不是终极方案**)

[Lib/radio/semtech/sx1280.c:107-135](Lib/radio/semtech/sx1280.c#L107-L135) 里 `SX1280Init()` 做了这件事:

```c
SX1280Reset();   /* 调 NRESET 但没用 — 模组拉高了 */

/* NRESET 没用,而且 chip 可能还在 sleep,BUSY 卡高 */
if (RADIO_READ_BUSY_PIN()) {
    /* 绕过 BUSY 直接发 SetStandby(STDBY_RC) 命令 */
    uint8_t cmd[2] = { 0x80, 0x00 };  /* SET_STANDBY, STDBY_RC */
    RADIO_CLR_NSS_PIN();
    delay_us(10);
    HAL_SPI_TransmitReceive(&hspi1, cmd, dummy, 2, 100);
    RADIO_SET_NSS_PIN();

    /* 最多等 5ms 让 chip 进入 STDBY_RC */
    for (int i = 0; i < 50; i++) {
        delay_us(100);
        if (!RADIO_READ_BUSY_PIN()) break;
    }
}
```

这段代码在每次 MCU boot 时都跑,假装"通过 SPI 发 SetStandby 来替代 NRESET reset"。**正常 boot 大多数能起来,但 chip 进入未定义状态后这条路救不回来**。

### 6.4 ⚠️ PCB 设计必须做的修复

根据用什么 SX1280 形态,有三个方案:

#### 方案 A:用裸 SX1280 芯片(QFN24)

最干净,推荐第二版用:

```
                    VDD (3.3V)
                     │
                    [10kΩ]
                     │
MCU PA0  ───────────┴─────── SX1280 NRESET pin
              (中间无其他元件)
```

要点:
- **10kΩ pull-up 到 VDD**(SX1280 datasheet 推荐值)
- **trace 短直**,< 10mm
- **不加电容,不加二极管**(芯片内部自己有保护)
- 别把 NRESET 接到上电复位电路,SX1280 不需要

#### 方案 B:继续用 DLP-RFS1280 模组,但**改硬件**

如果第一版还用模组:

**步骤 1**: 拿掉模组上原有的 NRESET 上拉/保护电路。具体做法:
- 找 DLP-RFS1280 模组的 NRESET 引脚(看模组 datasheet pinout)
- 用万用表 / 示波器测试 NRESET 引脚和 VDD 之间的等效电阻
- 如果远小于 50kΩ(说明有强上拉),拆掉模组上对应的电阻(需要热风枪)
- 这一步**风险**: 拆错件可能让模组其他功能挂掉

**步骤 2**: 设计你自己 PCB 的 NRESET 网络
```
                    VDD (3.3V)
                     │
                    [10kΩ]  ← 你 PCB 上加的弱上拉
                     │
MCU PA0  ─────────── ┴ ─────── DLP-RFS1280 NRESET pin
                                (绕过模组内部上拉,直接打到芯片)
```

#### 方案 C:换其他 SX1280 模组

如果方案 B 太麻烦(也不愿意拆元件),直接换一款 NRESET 引出更干净的模组,例如:
- **Murata LBAA0XV1SJ** (1ZM series) — 自带匹配 + 屏蔽,NRESET 标准引出
- **Insight SiP ISP4520-S0** (但是这是 LoRa,不是 SX1280)
- **EBYTE E28-2G4M20S** — 国产 2.4G LoRa/FLRC,NRESET 引脚标记清楚

⚠️ **换模组的代价**:封装 / pinout 会和 DLP-RFS1280 不同,要重画 PCB 焊盘。

### 6.5 投板前必做的验证

无论用哪个方案,**焊好板子第一件事是测 NRESET**:

1. 串口上焊一根线到 NRESET 引脚
2. 程序里加临时代码:启动后每秒翻转 NRESET 一次
   ```c
   while(1) {
       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
       HAL_Delay(500);
       HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
       HAL_Delay(500);
   }
   ```
3. 用万用表 / 示波器在 SX1280 NRESET pin 上看
   - **正常**: 电压在 0V ↔ 3.3V 间方波切换
   - **不正常**: 一直停在 3.3V(说明模组/电路还在强拉高)
4. 如果不正常,**先解决 NRESET,再焊其他东西** — 否则代码里那段 BUSY-bypass workaround 救不回奇怪状态

### 6.6 总结

| 维度 | DLP-RFS1280 现状 | PCB 修复后期望 |
|---|---|---|
| NRESET 可控 | ❌ 拉不下来 | ✅ MCU 直接控制 |
| 软件 reset | ❌ 不工作(靠 SPI workaround) | ✅ 标准流程 |
| 异常恢复 | ❌ 必须拔 USB | ✅ 软件可强制 reset |
| 代码 workaround | ⚠️ 还得保留(防止 reset 真的失败) | ✅ 可作为安全网保留 |

---

## 7. RF 设计(最复杂的部分)

**第一版强烈建议用 DLP-RFS1280 模组**。它把这些都打包好了:
- 2.4 GHz 阻抗匹配电路
- RF 屏蔽罩
- 板载天线
- 旁路电容

**如果**你要用裸 SX1280 芯片:
- 2.4 GHz 的射频走线必须是 **50Ω 微带线**(controlled impedance) — 你需要让 PCB 厂帮你算线宽,告诉他们 stackup
- 天线匹配电路 (π 网络或 L 网络) — 参考 Semtech reference design AN1200.49
- RF trace 长度 < 5mm 最理想,**绝对不要**穿过 GND 分割
- 天线选择: 板载 chip 天线(WE 7488010003 等) 或 IPEX 接外置
- RF 走线下方 GND 平面必须完整,不能有 trace 穿过

**新手建议**:第一版 PCB 用模组,第二版再考虑裸片。

---

## 8. 电源

| 项 | 要求 |
|---|---|
| MCU VDD | 3.3V,典型电流 ~30mA(满载),低功耗 < 1mA |
| SX1280 VDD | 3.3V,**TX 峰值 ~70mA** @ +12.5dBm |
| 电源芯片 | 推荐 **LDO** ≥ 200mA(比如 AMS1117-3.3 或更低噪声的 TPS7A02) |
| MCU 去耦 | 每个 VDD 引脚一个 100nF + 整体一个 4.7µF |
| SX1280 去耦 | **每个 VDD 一个 100nF + 一个 10nF + 1µF** — RF 芯片对电源噪声敏感 |
| USB 供电 | 如果板载 ST-LINK,会用 5V → LDO 降到 3.3V |

**关键**: SX1280 和 MCU **共地**, 但电源走线尽量分开(MCU 数字噪声不要污染 RF)。

---

## 9. 调试 / 编程接口

PCB 上要留 SWD 调试座(标准 5-pin):

| 引脚 | 信号 | MCU 引脚 |
|---|---|---|
| 1 | VTREF (3.3V) | 3V3 |
| 2 | SWCLK | PA14 |
| 3 | GND | GND |
| 4 | SWDIO | PA13 |
| 5 | NRST | NRST(MCU 的) |
| ~6~ | ~SWO~ | **不接**(见第 5 节) |

接口规格推荐 **TC2030-IDC**(6 pin pogo,不占空间) 或 **2x5 标准 SWD 排针**。

---

## 10. 串口(用于看 LWB 日志)

代码用 **USART2** 跑 115200, 8N1:

| MCU 引脚 | 信号 | 接 |
|---|---|---|
| PA2 | TX | UART转USB芯片 RXD,或留排针 |
| PA3 | RX | UART转USB芯片 TXD,或留排针 |

如果板载 ST-LINK 芯片(STM32F103C8 + 固件),它本身就带 USB-串口功能,把 PA2/PA3 接到 ST-LINK 的 VCP 输入即可,USB 一根线搞定调试 + 串口。

**新手建议**:第一版用外置 ST-LINK V2 + 单独 USB-TTL 模块,板上只留 SWD 排针 + UART 排针,简单可靠。

---

## 11. 时钟源(代码已配死)

| 时钟 | 来源 | PCB 上 |
|---|---|---|
| **SYSCLK** | HSE_BYPASS 8MHz (从 ST-LINK MCO 来) | 你的板没 ST-LINK 时,需要**外接 8MHz 有源晶振**到 OSC_IN(PH0/PF0)。或换成 HSI 内部 16MHz(改代码) |
| **LSE** | 32.768 kHz 晶振 | PC14/PC15 焊一个 6pF 表晶 + 2×12pF 负载电容 |
| **HSI** | 内部 16MHz(USART2 用) | 无需外部元件 |

**新手建议**: PCB 板载 8MHz 有源晶振(比表晶简单),不依赖 ST-LINK MCO。

---

## 12. 其他被代码用到的 GPIO(建议留 test point)

| MCU 引脚 | 用途 | 重要度 |
|---|---|---|
| PA11 | RADIO_TX_IND(逻辑分析仪观察 TX) | 低,debug 用 |
| PA12 | RADIO_RX_IND(逻辑分析仪观察 RX) | 低,debug 用 |
| 各 LED 引脚 | 看代码 `Lib/system/gpio.c` 找 LED 定义 | 加几个状态 LED 利于现场调试 |

---

## 13. 推荐 PCB 设计流程(新手向)

1. **画原理图**: 按上面的 pinout 连。SX1280 模组 + MCU + LDO + SWD + USB + 串口接口
2. **DRC 检查**:
   - 每个 VDD 都有去耦电容?
   - 晶振有负载电容?
   - 留了 BOOT0 下拉?
3. **PCB 布局**:
   - MCU 在中间
   - SX1280 模组放一边,天线远离 MCU 和电源
   - 电源芯片放另一边
   - **2 层板就够**,但 GND 层必须完整覆盖
4. **走线优先级**:
   1. RF 走线(50Ω 阻抗,短,直)
   2. 晶振走线(短,远离 RF)
   3. SPI 走线(中等优先,GND 邻接)
   4. 其他 GPIO 随意
5. **打样前**:让有经验的人 review 一次

---

## 14. 必备文档

- **STM32L476RG datasheet** + **NUCLEO-L476RG 原理图**(ST 官网) — 找 MCU pinout 和参考线路
- **SX1280 datasheet** (Semtech) — 第 7 章 "Application Information" 有参考电路
- **DLP-RFS1280 datasheet** — 模组 pinout 和封装尺寸
- **Semtech AN1200.49** "Reference Design for SX1280" — 裸片设计参考(如果不用模组)
- 本仓库 [README.md](README.md) 第 80-126 行 — 当前已验证的 pinout 全表

---

## 15. 给你的建议(顺序)

### 第一版(目标:能跑代码即可)

- DLP-RFS1280 模组 + STM32L476RG + 外置 ST-LINK
- 板上只留 SWD 排针 + UART 排针 + 4 个 LED + 几个 test point
- 解决 NRESET 直连问题(第 6 节)
- 板载 8MHz 有源晶振 + 32.768kHz 表晶
- 单层信号 + 完整 GND 平面,2 层板即可
- 提前 1-2 周送嘉立创 / JLCPCB 打样(便宜 + 快)

### 第二版(目标:小型化/集成化)

- 裸 SX1280 + 板载 ST-LINK + USB 直连
- 处理 RF 阻抗匹配

### 不建议

- 第一版就用裸 SX1280 + 自己设计 RF 匹配(失败率太高,毕设时间不够)

---

## 附录:已验证 pinout 速查表

```
SX1280 ←→ STM32L476RG  (代码 fd0099f 之后)
================================
NRESET    →  PA0
BUSY      →  PB3   (注意 SWO 冲突)
DIO1      →  PB4 + PB11 (同一个 net)
MISO      →  PA6
MOSI      →  PA7
SCK       →  PA5
NSS       →  PA8
ANTSEL    →  PA9

调试接口
================================
SWDIO     →  PA13
SWCLK     →  PA14
NRST      →  MCU 的 NRST 引脚
SWO       →  不接

串口
================================
USART2_TX →  PA2
USART2_RX →  PA3

时钟
================================
LSE_IN    →  PC14  (32.768 kHz 表晶)
LSE_OUT   →  PC15
HSE_IN    →  PH0   (8 MHz 有源晶振 or HSE_BYPASS)
HSE_OUT   →  PH1   (无源晶振时用,有源晶振时悬空)

可选 debug test point
================================
RADIO_TX_IND  →  PA11
RADIO_RX_IND  →  PA12
```

---

*生成日期: 2026-05-22*
*基于 commit 7a0810d 时代码状态*
