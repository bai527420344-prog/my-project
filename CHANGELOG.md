# Changelog

All changes relative to the original GitLab version (DPP2 ComBoard target) to port the LWB project to the **NUCLEO-L433RC-P + SX1262 Evaluation Board** hardware.

## [Port] Radio GPIO Pin Remapping

**Files**: `Inc/main.h`, `Src/main.c` (MX_GPIO_Init)

**Why**: The original pin assignments target the DPP2 ComBoard PCB. The NUCLEO-L433RC-P connects to the SX1262 eval board via Arduino headers, which maps the radio signals to different physical pins.

| Signal | Original (ComBoard) | New (Nucleo + SX1262 Eval) |
|--------|---------------------|---------------------------|
| RADIO_NSS | PB12 | PC7 (Arduino D9) |
| RADIO_NRESET | PA8 | PA0 (Arduino A0) |
| RADIO_BUSY | PA11 | PB3 (Arduino D3) |
| RADIO_ANT_SW | PA12 | PB6 (Arduino D10) |

SPI pins (PB13/PB14/PB15) and DIO1 (PA15) were already correct and unchanged.

**MX_GPIO_Init changes**:
- Split grouped GPIO init calls because remapped pins now span different ports (e.g. RADIO_NSS moved from GPIOB to GPIOC).
- Removed RADIO_BUSY (PB3) from COM_GPIO2 output init group. Without this fix, PB3 was configured as output after being set as input, preventing the MCU from reading the SX1262 BUSY signal and causing the firmware to hang.
- Removed PB6 from the analog pin group since it is now used as RADIO_ANT_SW output.
- Removed PA2 from the analog pin group since it is now used as UART TX.

## [Port] UART: USART1 (PA9/PA10) -> USART2 (PA2/PA3)

**Files**: `Inc/main.h`, `Src/main.c`, `Src/stm32l4xx_hal_msp.c`, `Src/stm32l4xx_it.c`, `Lib/system/platform.h`

**Why**: The NUCLEO-L433RC-P board's ST-Link Virtual COM Port (VCP) is wired to PA2/PA3 (USART2) via solder bridges SB13/SB14. The original code used USART1 on PA9/PA10, which has no physical connection to the USB VCP on this board.

**Changes**:
- `Inc/main.h`: UART_TX pin PA9 -> PA2, UART_RX pin PA10 -> PA3.
- `Src/main.c`: Renamed `huart1` / `hdma_usart1_rx` / `hdma_usart1_tx` to `huart2` / `hdma_usart2_rx` / `hdma_usart2_tx`. Changed USART instance from USART1 to USART2. Changed baud rate from 1000000 to 115200. Changed peripheral clock selection from `RCC_PERIPHCLK_USART1` to `RCC_PERIPHCLK_USART2`. Changed DMA channels from Ch4/Ch5 to Ch7/Ch6.
- `Src/stm32l4xx_hal_msp.c`: Updated UART MSP Init/DeInit to use USART2, GPIO AF7_USART2, DMA1_Channel6 (RX) and DMA1_Channel7 (TX), USART2_IRQn.
- `Src/stm32l4xx_it.c`: Replaced DMA1_Channel4/5 IRQ handlers with DMA1_Channel7/6. Replaced USART1_IRQHandler with USART2_IRQHandler.
- `Lib/system/platform.h`: Changed `#define UART huart1` to `#define UART huart2`.

## [Port] Clock Configuration: HSE -> HSI

**File**: `Src/main.c` (SystemClock_Config)

**Why**: The original ComBoard has an external 8 MHz HSE crystal. The NUCLEO-L433RC-P does **not** have an HSE crystal soldered on the board. Without this change, `HAL_RCC_OscConfig()` waits indefinitely for the HSE oscillator to become ready, and the firmware never reaches `main()`.

**Changes**:
- PLL source changed from `RCC_PLLSOURCE_HSE` to `RCC_PLLSOURCE_HSI`.
- HSE state changed from `RCC_HSE_ON` to `RCC_HSE_OFF`.
- PLLM divider changed from 1 to 2, to maintain the same 32 MHz SYSCLK: `16 MHz (HSI) / 2 * 8 / 2 = 32 MHz` (previously `8 MHz (HSE) / 1 * 8 / 2 = 32 MHz`).

## [Port] TIM2 MSP: Remove COM_TREQ (PA3) from TIM2_CH4

**File**: `Src/stm32l4xx_hal_msp.c` (HAL_TIM_Base_MspInit)

**Why**: PA3 was previously used as COM_TREQ (TIM2 Channel 4 input), a ComBoard-specific signal. After the UART change, PA3 is now USART2_RX. The TIM2 alternate function configuration on PA3 would conflict with the UART function.

**Change**: Removed the `COM_TREQ_Pin` GPIO AF init block from TIM2 MSP Init. TIM2_CH1 on PA15 (RADIO_DIO1) remains unchanged.

## [Port] TX/RX Debug Indicator Pins

**File**: `Inc/main.h`

**Why**: The `RADIO_TX_START_IND()` / `RADIO_RX_START_IND()` macros in `app_config.h` use `PIN_SET(COM_GPIO2)` and `PIN_SET(COM_PROG2)` to toggle GPIO pins during radio TX/RX for logic analyzer observation.

The original pin assignments conflict on the Nucleo hardware:
- COM_GPIO2 was PB3 = same pin as RADIO_BUSY (input), so PIN_SET has no visible effect.
- COM_PROG2 was PA13 = SWDIO debug pin, occupied by ST-Link.

**Changes**:
- COM_GPIO2 (TX indicator) remapped from PB3 to **PA11** (Morpho CN10 pin 14).
- COM_PROG2 (RX indicator) remapped from PA13 to **PA12** (Morpho CN10 pin 12).

These pins are on the Morpho connector only, avoiding conflicts with the SX1262 eval board on the Arduino headers.

## Known Issue: lpm.c Hardcoded USART1 IRQ References

**File**: `Lib/system/lpm.c` (lines 231-233, 325-327) -- NOT modified (protocol/system library)

The low-power mode prepare/resume functions contain hardcoded references to `USART1_IRQn` and `DMA1_Channel4/5_IRQn`. These should be updated to `USART2_IRQn` and `DMA1_Channel6/7_IRQn` for correct interrupt management across sleep/wake cycles. This does not prevent basic operation but may affect UART reliability after deep sleep wake-up.

## Files Modified (Summary)

| File | Changes |
|------|---------|
| `Inc/main.h` | Radio GPIO pins, UART pins, indicator pins |
| `Src/main.c` | Clock config (HSE->HSI), UART init (USART2), GPIO init (pin regrouping), DMA channels, TIM2 prescaler (5→3) |
| `Src/stm32l4xx_hal_msp.c` | UART MSP (USART2), TIM2 MSP (remove COM_TREQ) |
| `Src/stm32l4xx_it.c` | DMA IRQ handlers (Ch6/Ch7), UART IRQ handler (USART2) |
| `Lib/system/platform.h` | UART macro (huart1 -> huart2) |

## [Fix] TIM2 (HS Timer) Prescaler: 5 → 3

**File**: `Src/main.c` (MX_TIM2_Init)

**Why**: The HS timer (TIM2) prescaler was set to 5, which was correct for the original ComBoard with a 48 MHz APB1 timer clock (`48 MHz / 6 = 8 MHz`). On the NUCLEO-L433RC-P, SYSCLK = 32 MHz (HSI-PLL), so the APB1 timer clock is 32 MHz. With prescaler 5, TIM2 ran at `32 MHz / 6 = 5.33 MHz` instead of the expected 8 MHz (`HS_TIMER_FREQUENCY` in `Lib/time/hs_timer.h`).

This 33% frequency mismatch caused all Gloria timing calculations (slot timing, flood duration, `reconstructed_marker` for `t_ref` reconstruction) to be systematically wrong. With N_TX=1 the error was small enough to be tolerated, but with N_TX=2 the accumulated timing offset exceeded the guard time, causing source nodes to miss the host's schedule transmissions.

**Change**: TIM2 prescaler changed from 5 to **3** → `32 MHz / (3+1) = 8 MHz`, matching `HS_TIMER_FREQUENCY`.

## [Config] LWB Guard Times

**File**: `Inc/app_config.h`

**Why**: Slightly increased guard times above default values to account for HSI oscillator drift (HSI is less accurate than the HSE crystal used on the original ComBoard).

**Changes**:
- `LWB_T_GUARD_ROUND`: default 1ms → 3ms
- `LWB_T_GUARD_ROUND_2`: default 5ms → 10ms
- `LWB_T_GUARD_SLOT`: unchanged (default 0.25ms)

## Files NOT Modified

All protocol and radio driver code remains untouched:
- `Lib/protocol/lwb/*` -- LWB protocol
- `Lib/protocol/gloria/*` -- GLORIA flooding protocol
- `Lib/radio/semtech/*` -- SX126x driver
- `Lib/radio/radio.c`, `radio_helpers.c`, `radio_constants.c` -- Radio HAL
| `Inc/app_config.h` | LWB guard times (GUARD_ROUND 1ms→3ms, GUARD_ROUND_2 5ms→10ms) |
