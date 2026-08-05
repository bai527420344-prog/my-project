# L476 + SX1280 双板型切换与硬件调试指南

This document describes the checks and firmware settings needed to bring up the
custom STM32L476 + SX1280 PCB while keeping compatibility with the legacy DPP2
ComBoard J400 interface.

The firmware is still used on the NUCLEO-L476RG development setup, so board
specific behavior must be selected by configuration instead of hard-coded for
only one board. 本文也是本工程唯一的板型切换与上板验证说明；原
`最终剩余更改.md` 的内容已经合并到这里。

## 快速结论

同一套源码支持两种硬件，板型差异集中在 `Inc/board_config.h`：

| `BOARD_TYPE` | 硬件 | 当前状态 |
|---:|---|---|
| `0` | 自定义 STM32L476 + SX1280 ComBoard PCB | 两种目标均编译通过；PCB 尚未制作，只完成编译级验证 |
| `1` | NUCLEO-L476RG + DLP-RFS1280 杜邦线开发环境 | 默认目标；已完成烧录、LWB 入网和 STOP2 功耗实测 |

推荐始终使用以下快捷目标：

```bash
# 开发板：BOARD_TYPE=1
make nucleo

# 自定义 PCB：BOARD_TYPE=0
make custom
```

这两个目标都会先执行 `clean`，因此不会误用另一种板型留下的目标文件。若不用
快捷目标，则必须显式清理：

```bash
make clean
make BOARD_TYPE=1

make clean
make BOARD_TYPE=0
```

生成文件均位于 `build/`：

```text
build/comboard_lwb.bin
build/comboard_lwb.hex
build/comboard_lwb.elf
```

`BOARD_TYPE` 只选择硬件差异，不决定设备在网络中的身份。`NODE_ID`、Host/Node
身份、调制方式、发射功率、LWB 周期、重传次数和日志开关仍在
`Inc/app_config.h` 中独立配置。这样同一块 Nucleo 或自定义 PCB 都能按实验需要
烧录成 Host 或 Node。

## 1. Board Targets

The project currently has two hardware targets:

| Target | Purpose | Main clock assumption |
|---|---|---|
| NUCLEO-L476RG development setup | Current development and radio testing | HSE bypass from ST-LINK MCO |
| Custom L476 + SX1280 ComBoard-compatible PCB | Final/custom board with legacy J400 compatibility | 12 MHz crystal on HOSC_IN/HOSC_OUT |

The same firmware should support both targets through a board selection macro.

## 2. J400 Compatibility

The custom PCB must keep the legacy DPP2 ComBoard J400 pinout. J400 is the
26-pin Molex board-to-board connector used by the old ComBoard.

| Pin | Signal | Pin | Signal |
|---:|---|---:|---|
| 1 | COM_VCC | 2 | BOLT_VCC |
| 3 | BOLT_RXD | 4 | BOLT_RST |
| 5 | BOLT_TXD | 6 | BOLT_PROG |
| 7 | BOLT_GPIO1 | 8 | BOLT_GPIO2 |
| 9 | APP_ACK | 10 | APP_MISO |
| 11 | APP_REQ | 12 | APP_MOSI |
| 13 | APP_MODE | 14 | APP_SCK |
| 15 | APP_IND | 16 | COM_TREQ |
| 17 | COM_IND | 18 | COM_GPIO1 |
| 19 | COM_GPIO2 | 20 | COM_PROG2 |
| 21 | COM_TXD | 22 | COM_PROG |
| 23 | COM_RXD | 24 | COM_RST |
| 25 | GND | 26 | GND |

Before PCB routing, verify the connector footprint:

- The footprint is the correct 26-pin Molex board-to-board connector.
- Pin 1 orientation matches the legacy ComBoard.
- The connector is placed on the correct PCB side.
- Mated height is compatible with the old board, approximately 4.5 mm.
- J400 mechanical position does not prevent mating with the legacy board.

## 3. Required Net Mapping

The latest schematic netlist should contain these connections.

### STM32 and J400

| J400 net | STM32 function | Current mapping |
|---|---|---|
| COM_TXD | USART2_TX | PA2 |
| COM_RXD | USART2_RX | PA3 |
| COM_IND | BOLT_IND input | PB7 |
| APP_IND | APP_IND input | PA4 |
| COM_PROG2 | SWDIO | PA13 |
| COM_PROG | SWCLK | PA14 |
| COM_RST | NRST | NRST |
| COM_TREQ | time request input | PA1 |
| COM_GPIO1 | debug / extension | PB5 |
| COM_GPIO2 | debug / TX indicator | PA11 |

### SX1280 Radio

| Radio net | STM32 pin | Notes |
|---|---|---|
| RADIO_SCK | PA5 | SPI1_SCK |
| RADIO_MISO | PA6 | SPI1_MISO |
| RADIO_MOSI | PA7 | SPI1_MOSI |
| RADIO_NSS | PA8 | chip select |
| RADIO_NRESET | PA0 | radio reset |
| RADIO_BUSY | PB3 | busy input |
| RADIO_DIO1 | PB4 and PB11 | PB4 is EXTI wakeup, PB11 is timer capture |

The shared RADIO_DIO1 connection to PB4 and PB11 is intentional for the current
firmware timing model.

## 4. Firmware Board Mode

Board selection is implemented centrally in `Inc/board_config.h`:

```c
#define BOARD_CUSTOM_COMBOARD      0
#define BOARD_NUCLEO_L476          1

#ifndef BOARD_TYPE
#define BOARD_TYPE BOARD_NUCLEO_L476
#endif
```

Use `make nucleo` for the development board and `make custom` for the final
PCB. Both convenience targets clean the previous build before compiling.

### 4.1 为什么必须切换板型

两种硬件虽然都使用 STM32L476 和 SX1280，但不能共用完全相同的底层初始化：

- Nucleo 的目标 MCU 从 ST-LINK MCO 获得 8 MHz 有源时钟，因此必须使用
  `RCC_HSE_BYPASS`。
- 自定义 PCB 使用 12 MHz 无源晶体，因此必须使用 `RCC_HSE_ON`。
- DLP-RFS1280 模块具有 `ANTSEL` 控制和 100 kΩ 下拉；自定义 PCB 的射频通路
  固定连接外置 SMA，不存在 `ANTSEL` 网络。
- 自定义 PCB 的 PA13/PA14 通过 J400 用作 SWDIO/SWCLK，不能像开发板上的
  PA12/PC4 一样作为普通活动指示 GPIO。

若继续手工改这些位置，很容易出现时钟计算错误、睡眠漏电、烧录接口被 GPIO
占用或错误选择天线。因此板型差异必须由一个编译选项统一控制。

### 4.2 自动切换内容

| 项目 | `BOARD_TYPE=0`：自定义 PCB | `BOARD_TYPE=1`：Nucleo + DLP |
|---|---|---|
| HSE 硬件 | 12 MHz 无源晶体 | ST-LINK 8 MHz MCO，SB16/SB50 连通 |
| HSE 模式 | `RCC_HSE_ON` | `RCC_HSE_BYPASS` |
| `HSE_VALUE` | 12,000,000 Hz | 8,000,000 Hz |
| PLL | M=3、N=24、R=2 | M=2、N=24、R=2 |
| SYSCLK | 48 MHz | 48 MHz |
| TIM2 高速计时 | 8 MHz | 8 MHz |
| `RADIO_ANTSEL` | 硬件不存在；不定义、不初始化、不驱动 | PA9；DLP 模块天线选择 |
| `COM_PROG2` | PA13/SWDIO，只用于 SWD | PA12，可作 RX indicator |
| `COM_PROG` | PA14/SWCLK，只用于 SWD | PC4，可作开发 GPIO |
| RX indicator | 禁用 | 启用 |
| 未接 BOLT/baseboard 输入 | 按 PCB 网络，不额外加下拉 | 使用内部下拉，避免杜邦线输入悬空 |

两个板型都保持 48 MHz SYSCLK，因为现有 TIM2 使用 `Prescaler=5`，需要由
48 MHz 定时器时钟产生 LWB 使用的 8 MHz 高精度计时基准。板型切换不能只改
`HSE_VALUE`；HSE 模式、PLL 除数和 CMSIS/HAL 的频率定义必须一起切换。

### 4.3 不随板型切换的配置

以下项目属于网络角色或实验条件，不能由 `BOARD_TYPE` 隐式改变：

- `NODE_ID`、`HOST_ID` 和 Host/Node 身份；
- GFSK/FLRC/LoRa 调制方式与发射功率；
- `LWB_N_TX`、`LWB_NUM_HOPS` 和数据产生周期；
- `LOG_ENABLE`、`CLI_ENABLE`、测试 GPIO 标记；
- STOP2 策略。

因此，切换板型后的标准操作是：先选择硬件目标，再单独核对
`Inc/app_config.h` 中本次实验所需的网络角色和参数。

## 5. Implemented Firmware Behavior for Custom PCB

### 5.1 BOLT indication line

This change has already been applied in the current firmware:

```c
#define BOLT_IND_Pin       GPIO_PIN_7
#define BOLT_IND_GPIO_Port GPIOB
```

`PB7` must be configured as GPIO input and must not be configured as unused
analog. This matches:

```text
COM_IND -> PB7 -> firmware BOLT_IND
```

### 5.2 HSE clock mode

The NUCLEO setup uses an 8 MHz ST-LINK MCO in HSE bypass mode. The custom PCB
uses a 12 MHz passive crystal in HSE oscillator mode. Both PLL configurations
produce a 48 MHz SYSCLK and therefore preserve the required 8 MHz TIM2 clock.

Use board-specific clock setup:

```c
#if BOARD_TYPE == BOARD_CUSTOM_COMBOARD
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
#else
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
#endif
```

`HSE_VALUE`, HSE mode and PLL divisors are selected together by
`board_config.h`; they must not be changed independently.

### 5.3 COM_PROG and COM_PROG2

For legacy J400 compatibility:

```text
COM_PROG2 -> PA13 / SWDIO
COM_PROG  -> PA14 / SWCLK
COM_RST   -> NRST
```

On the custom PCB these pins are programming/debug pins and must not be driven
as normal GPIO outputs.

In custom PCB mode:

- Do not initialize PA13 or PA14 as output GPIO.
- Do not use `COM_PROG` or `COM_PROG2` as radio activity indicators.
- Keep PA13/PA14 available for SWD.

For NUCLEO development mode, the current debug indicator behavior can remain.

Recommended custom PCB override:

```c
#if BOARD_TYPE == BOARD_CUSTOM_COMBOARD
  #define RADIO_RX_START_IND()
  #define RADIO_RX_STOP_IND()
#else
  #define RADIO_RX_START_IND() PIN_SET(COM_PROG2)
  #define RADIO_RX_STOP_IND()  PIN_CLR(COM_PROG2)
#endif
```

### 5.4 RADIO_ANTSEL

The current custom PCB netlist does not route `RADIO_ANTSEL`.

In custom PCB mode:

- Do not initialize `RADIO_ANTSEL` as a GPIO output unless the PCB adds an
  antenna switch control net.
- Disable or guard calls that drive `RADIO_ANTSEL`.

In NUCLEO development mode, keep the existing behavior if the test setup still
uses that signal.

### 5.5 UART

The custom PCB maps the J400 COM UART to the same pins used by the current
firmware:

```text
COM_TXD -> PA2 -> USART2_TX
COM_RXD -> PA3 -> USART2_RX
```

The existing USART2 firmware can be reused. Serial settings:

```text
115200 baud, 8 data bits, no parity, 1 stop bit
```

### 5.6 Completed firmware change table

| Item | Code location | Implemented behavior | Reason |
|---|---|---|---|
| Board selection | `Inc/board_config.h`, `Makefile` | `0=custom`, `1=Nucleo`; `make custom` and `make nucleo` are available. | One firmware tree supports both boards. |
| HSE mode | `Inc/board_config.h`, `Src/main.c` | Custom: 12 MHz/HSE ON/M=3; Nucleo: 8 MHz/bypass/M=2; both use N=24, R=2. | Both boards run at a real 48 MHz SYSCLK. |
| SWD pins on custom PCB | `Inc/main.h`, `Src/main.c`, `Lib/system/gpio.c` | In custom PCB mode, map `COM_PROG2` to `PA13/SWDIO` and `COM_PROG` to `PA14/SWCLK`, and do not initialize them as GPIO outputs. | J400 compatibility requires these nets to remain available for programming/debug. |
| RX indicator macros | `Inc/app_config.h` | In custom PCB mode, make `RADIO_RX_START_IND()` and `RADIO_RX_STOP_IND()` empty. Keep the existing NUCLEO behavior. | `COM_PROG2` is SWDIO on the custom PCB and must not be used as a radio RX indicator. |
| Antenna select | `Src/main.c`, `Lib/radio/semtech/sx1280-board.c` | In custom PCB mode, do not initialize or drive `RADIO_ANTSEL`. Keep the existing NUCLEO behavior. | The current custom PCB netlist does not route `RADIO_ANTSEL`. |
| COM_GPIO2 port | `Src/main.c` | PA11 is written through GPIOA instead of accidentally writing PB11. | PB11 is the radio DIO1 timer-capture input. |

## 6. Pre-routing Checklist

Complete this checklist before PCB routing:

- J400 pinout matches the legacy DPP2 ComBoard connector.
- J400 footprint, side, pin 1 orientation, and mated height are verified.
- COM_TXD goes to PA2.
- COM_RXD goes to PA3.
- COM_IND goes to PB7.
- APP_IND goes to PA4.
- COM_PROG2 goes to PA13/SWDIO.
- COM_PROG goes to PA14/SWCLK.
- COM_RST goes to NRST.
- RADIO_NRESET goes to PA0 and does not share a net with COM_IND.
- RADIO SPI nets go to PA5/PA6/PA7/PA8.
- RADIO_BUSY goes to PB3.
- RADIO_DIO1 goes to PB4 and PB11.
- HOSC_IN/HOSC_OUT connect to the 12 MHz crystal.
- OSC_IN/OSC_OUT connect to the 32.768 kHz crystal.
- 3.3 V and GND are present on J400 and all active ICs.

## 7. Assembly Bring-up Checklist

After the PCB is assembled:

1. Check for shorts between 3.3 V and GND before applying power.
2. Power the board and verify the 3.3 V rail.
3. Connect ST-LINK through SWD:
   - PA13 / SWDIO
   - PA14 / SWCLK
   - NRST
   - GND
   - 3.3 V reference
4. Confirm that ST-LINK can detect the STM32L476.
5. Build firmware with `make custom`.
6. Flash the firmware.
7. Open the UART console on COM_TXD/COM_RXD at 115200 baud.
8. Confirm boot log output appears.
9. Confirm SX1280 SPI access through radio CLI or boot logs.
10. Confirm `RADIO_BUSY` is not stuck high.
11. Confirm `RADIO_DIO1` toggles during TX/RX activity.
12. Run host/node LWB communication and check for expected network logs.

## 8. Expected Firmware Configuration

Current default network/radio parameters are defined in `Inc/app_config.h`:

| Parameter | Expected value |
|---|---|
| HOST_ID | 1 |
| NODE_ID | currently 2 for non-FlockLab; independent of board type |
| LWB_NETWORK_ID | 0x4444 |
| GLORIA_INTERFACE_RF_BAND | 24, 2450 MHz |
| GLORIA_INTERFACE_MODULATION | 11, FLRC 260 kbit/s |
| GLORIA_INTERFACE_POWER | 10 dBm |
| LWB_SCHED_PERIOD | 15 s |
| LWB_N_TX | 2 |
| LWB_NUM_HOPS | 6 |

## 8.1 已完成的实机回归验证（2026-07-15）

`BOARD_TYPE=1` 已在 NUCLEO-L476RG + DLP-RFS1280 Node2 上完成实机验证：

- `make nucleo` 编译成功，Node2 固件写入并通过 flash verification；
- 修改前烧录的 L476 + SX1280 Host 无需重新烧录；
- Host 启动日志确认调制索引 11，即 FLRC 260 kbit/s；
- Host 与 Node2 均使用网络 ID `0x4444`、15 s 周期、`n_tx=2`、6 hops；
- Host 从第 31 s 开始每隔 15 s 稳定输出 `1 msg rcvd from network`，持续到
  241 s 未见漏周期；
- PPK2 同时测得 Node2 每 15 s 一次通信事件，整节点睡眠基线约 10.65 µA。

这证明新的板型选择、时钟修正和低功耗修改没有破坏现有 LWB 空口兼容性。

`BOARD_TYPE=0` 目前只能声明“编译通过”。PCB 到货后必须按第 7 节完成 12 MHz
晶体、J400 SWD/reset、USART2、SX1280、LWB 和 STOP2 的硬件验证，不能提前写成
“实机验证完成”。

## 9. Troubleshooting

### No SWD connection

- Check PA13/PA14 are not driven as GPIO in custom PCB firmware.
- Check COM_PROG2/COM_PROG are connected to SWDIO/SWCLK.
- Check NRST and GND.
- Check target 3.3 V reference to ST-LINK.

### No UART output

- Check COM_TXD is connected to USB-UART RX.
- Check COM_RXD is connected to USB-UART TX.
- Check common GND.
- Check baud rate is 115200.
- Confirm firmware still uses USART2 on PA2/PA3.

### MCU does not start on custom PCB

- Check firmware uses `RCC_HSE_ON` for the custom PCB.
- Check the 12 MHz crystal and load capacitors.
- Check NRST pull-up.
- Check BOOT pin state.

### Radio does not respond

- Check RADIO_NSS, SCK, MISO, MOSI routing.
- Check RADIO_NRESET on PA0.
- Check RADIO_BUSY on PB3.
- Check 52 MHz SX1280 crystal.
- Check 3.3 V supply near the radio.

### BOLT is not detected

- Check COM_IND is connected to PB7.
- Check APP_IND is connected to PA4.
- Check COM_REQ, COM_ACK, COM_MODE, COM_SCK, COM_MOSI, COM_MISO.
- Confirm `BOLT_ENABLE` is enabled when testing BOLT functionality.

## 10. Notes

- The current custom PCB J400 mapping is compatible with the legacy DPP2
  ComBoard connector.
- The NUCLEO/custom hardware differences are selected at compile time through
  `BOARD_TYPE`; they are no longer manual source edits.
- Do not regenerate code from `comboard_lwb.ioc` without first updating the IOC
  pin mapping, because it may restore older pin assignments.
- 低功耗测试、问题定位和调试前后数据见 `论文/论文.md`。
