# LWB 移植报告：Comboard -> NUCLEO-L433RC-P + SX1262 Eval Board

## 移植状态：已成功

LWB 协议已在 NUCLEO-L433RC-P + SX1262 Eval Board 上成功运行。

## 硬件平台
- **MCU板**: NUCLEO-L433RC-P (STM32L433RCT6)
- **射频板**: Semtech SX1262 Evaluation Board (SX1262MB2CAS)
- **连接方式**: 通过 Arduino 排针连接（参考 flora_dpp_comboard README 接线表）

## 已完成的修改

### 1. 射频 GPIO 引脚重映射（main.h + main.c）

原始代码为 comboard PCB 设计，引脚不匹配 Nucleo+SX1262 eval board 的物理接线。

| 信号 | 原引脚 | 新引脚 | 状态 |
|------|--------|--------|------|
| RADIO_NSS | PB12 (GPIOB) | PC7 (GPIOC) | 已改 |
| RADIO_NRESET | PA8 (GPIOA) | PA0 (GPIOA) | 已改 |
| RADIO_BUSY | PA11 (GPIOA) | PB3 (GPIOB) | 已改 |
| RADIO_ANT_SW | PA12 (GPIOA) | PB6 (GPIOB) | 已改 |
| RADIO_SCK | PB13 | PB13 | 无需改 |
| RADIO_MISO | PB14 | PB14 | 无需改 |
| RADIO_MOSI | PB15 | PB15 | 无需改 |
| RADIO_DIO1 | PA15 | PA15 | 无需改 |

**修改文件**:
- `Inc/main.h` -- 4 个 #define 引脚定义
- `Src/main.c` MX_GPIO_Init() -- 由于引脚换了端口，原来按端口分组的 GPIO 初始化拆分为独立调用

### 2. UART 从 USART1 改为 USART2（走 ST-Link VCP）

Nucleo 板的 ST-Link 虚拟串口 (VCP) 连接在 PA2/PA3 (USART2)，而原代码使用 USART1 (PA9/PA10)。

| 项目 | 原来 | 现在 |
|------|------|------|
| UART 外设 | USART1 | USART2 |
| TX 引脚 | PA9 | PA2 |
| RX 引脚 | PA10 | PA3 |
| 波特率 | 1000000 | 115200 |
| DMA RX 通道 | DMA1_Channel5 | DMA1_Channel6 |
| DMA TX 通道 | DMA1_Channel4 | DMA1_Channel7 |

**修改文件**:
- `Inc/main.h` -- UART_TX/RX 引脚定义改为 PA2/PA3
- `Lib/system/platform.h` -- `#define UART huart1` -> `huart2`
- `Src/main.c` -- huart2 变量声明、MX_USART2_UART_Init()、PeriphClk 时钟源、DMA 通道中断优先级、MX_GPIO_Init 中 PA2 从 analog 移除
- `Src/stm32l4xx_hal_msp.c` -- UART MSP Init/DeInit 改为 USART2、GPIO AF7_USART2、DMA Ch6/Ch7、extern 声明
- `Src/stm32l4xx_it.c` -- DMA IRQ Handler 改为 Ch6/Ch7、USART2_IRQHandler、extern 声明

### 3. 引脚冲突处理

由于新引脚与 comboard 专用信号共用同一物理引脚：

| 冲突 | 处理 |
|------|------|
| RADIO_NRESET (PA0) = BOLT_IND (PA0) | BOLT_IND 先初始化为 input，RADIO_NRESET 后初始化为 output 覆盖之。Nucleo 上无 BOLT 硬件，无影响 |
| RADIO_BUSY (PB3) = COM_GPIO2 (PB3) | 已从 MX_GPIO_Init 中移除 COM_GPIO2 的 output 初始化，否则会覆盖 BUSY 的 input 配置导致挂死 |
| RADIO_ANT_SW (PB6) | 已从 analog 分组中移除 PB6 |
| UART_RX (PA3) = COM_TREQ (PA3) | 已从 TIM2 MSP Init 中移除 COM_TREQ 的 TIM2_CH4 AF 配置 |

### 4. 时钟配置修改（main.c SystemClock_Config）

原 comboard 使用 HSE 外部 8MHz 晶振，NUCLEO-L433RC-P 没有 HSE 晶振，改用 HSI 内部 16MHz。

| 项目 | 原来 | 现在 |
|------|------|------|
| PLL 时钟源 | HSE (8MHz) | HSI (16MHz) |
| HSE 状态 | RCC_HSE_ON | RCC_HSE_OFF |
| PLLM 分频 | 1 | 2 |
| SYSCLK | 32MHz | 32MHz（不变） |

公式：16MHz / 2 x 8 / 2 = 32MHz（与原来一致）

**修改文件**: `Src/main.c` SystemClock_Config()

## 未修改的文件（协议层，不可改）
- `Lib/protocol/lwb/*` -- LWB 协议
- `Lib/protocol/gloria/*` -- GLORIA 洪泛协议
- `Lib/radio/semtech/*` -- SX126x 驱动
- `Lib/radio/radio.c`, `radio_helpers.c`, `radio_constants.c`

## 修改过的文件清单
1. `Inc/main.h` -- 射频 GPIO + UART 引脚定义
2. `Src/main.c` -- MX_GPIO_Init, MX_USART2_UART_Init, DMA, 时钟, SystemClock_Config
3. `Src/stm32l4xx_hal_msp.c` -- UART/TIM2 MSP Init/DeInit
4. `Src/stm32l4xx_it.c` -- DMA + UART 中断处理
5. `Lib/system/platform.h` -- UART 宏定义

## 调试命令参考
```bash
# 编译
make -j4

# 烧录
st-flash --connect-under-reset write build/comboard_lwb.bin 0x08000000

# 串口监听 (Linux)
stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb raw -echo && cat /dev/ttyACM0

# hterm 配置: /dev/ttyACM0, 115200, 8N1
```

## 注意事项
- comboard 专用外设 (BOLT, BASEBOARD) 在 Nucleo 上不存在，相关宏定义保留但功能无效
- `app_config.h` 中 `PIN_SET(COM_GPIO2)` 等宏写 PB3 的 ODR，因为 PB3 是 input 模式所以无实际影响
- LSE (32.768kHz) 在 Nucleo 板上可用，LPTIM1 定时正常
- 低功耗模式 LP_MODE_STOP2 可能影响调试，如需持续串口输出可临时改为 LP_MODE_SLEEP
