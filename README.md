# LWB for NUCLEO-L476RG + SX1280

This repository is an `L476 + SX1280` migration project.

The original baseline was:

- `L433 + SX1262`
- then migrated to `L476 + SX1262`
- and is now being migrated to `L476 + SX1280` (`2.4 GHz`)

The current work keeps the original protocol logic as much as possible and only replaces the radio chip plus the required board-level adaptation.

## Hardware target selection

The same source tree supports both hardware versions:

```bash
# BOARD_TYPE=1: NUCLEO-L476 + DLP-RFS1280 (default)
make nucleo

# BOARD_TYPE=0: custom L476 + SX1280 ComBoard PCB
make custom
```

These targets clean before compiling so objects from different boards cannot be
mixed. Board-specific HSE frequency/mode, PLL, ANTSEL and J400 SWD behavior are
defined centrally in [Inc/board_config.h](Inc/board_config.h). Network role,
modulation and logging remain independent application settings.

### Board validation status

| Board setting | Current status | Required verification |
|---|---|---|
| `BOARD_TYPE=1` — NUCLEO-L476 + DLP-RFS1280 | Hardware-validated on 2026-07-15 | Node2 joined the existing L476+SX1280 Host and delivered one message every 15 s; whole-node STOP2 baseline was approximately 10.65 µA |
| `BOARD_TYPE=0` — custom L476 + SX1280 PCB | Compiles successfully; hardware not yet available | After PCB assembly, verify the 12 MHz HSE, J400 SWD/reset, USART2, SX1280 TX/RX, LWB joining and STOP2 power |

Until the custom PCB is manufactured, `BOARD_TYPE=0` being buildable means
compile-time validation only; it must not be described as hardware-validated.
The `BOARD_TYPE=1` regression has been completed using the existing Nucleo
host/node setup. See [COMMISSIONING.md](COMMISSIONING.md) and
[论文/论文.md](论文/论文.md) for the procedure and power measurements.

## Project Status (2026-05-21)

Migration `SX1262 → SX1280` is complete. All planned modules `R0`–`R6` and `F` have been validated end-to-end; `R7` (final cleanup) is in progress.

The three SX1280 modulations are all running over LWB with two physical boards (HOST + NODE, 15 s round period, n_tx=2):

| `GLORIA_INTERFACE_MODULATION` | Modulation | Status |
|---|---|---|
| `7`  | LoRa SF5             | validated |
| `8`  | GFSK 125 kbit/s      | validated |
| `10` | FLRC 260 kbit/s CR=1/2 | validated |
| `9`  | GFSK 250k              | table entry present, not end-to-end verified |
| `11`/`12` | FLRC 650k / 1300k | table entries present, not end-to-end verified |

The table contains 13 PHY configuration profiles (`0` through `12`): eight
LoRa spreading factors, two GFSK profiles, and three FLRC profiles. The former
SX1262-derived GFSK 200 kbit/s entry was removed because SX1280 has no native
200 kbit/s bitrate/bandwidth encoding; profiles that followed it moved down by
one index.

Switching between modulations only requires changing the value of `GLORIA_INTERFACE_MODULATION` in [Inc/app_config.h](Inc/app_config.h) and rebuilding both boards (`make clean && make -j4 && st-flash ...`). The boot log line `task_com: modulation index N: <name> <rate>` shows the active mode.

## Project Description

Project target:

- MCU: `STM32L476RG`
- Board: `NUCLEO-L476RG`
- Radio: `SX1280` (DLP-RFS1280 module)
- Band: `2.4 GHz ISM`
- Protocol stack: `LWB` (Glossy floods scheduled by LWB)

Goals achieved:

- bring up `SX1280` on the existing `L476` platform
- keep the existing `LWB / GLORIA / radio` logic structure
- complete the migration from `SX1262` to `SX1280`
- support all three SX1280 main modulations (LoRa, GFSK, FLRC) through the same LWB stack

## Hardware Baseline

The validated `L476` clock baseline used by this repository is:

- `SYSCLK` uses `HSE_BYPASS + PLL`
- `RTC / LSE` uses `LSE`
- `USART2` uses `HSI`

Board preparation for the current bring-up code:

- keep `SB55 OFF`
- keep `SB54 ON`
- keep `SB16 ON`
- keep `SB50 ON`

This routes `ST-LINK MCO 8 MHz` to the target MCU `HSE_BYPASS`.

`UM1724` documents the default `NUCLEO-L476RG` configuration differently, but this repository follows the actual validated board setup used by the current firmware.

## Current Firmware Signal Definitions

The radio-related pins currently used by the firmware are:

| Signal | MCU Pin | Description |
|---|---|---|
| `RADIO_NSS` | `PA8` | SPI chip select |
| `RADIO_SCK` | `PA5` | `SPI1_SCK` |
| `RADIO_MISO` | `PA6` | `SPI1_MISO` |
| `RADIO_MOSI` | `PA7` | `SPI1_MOSI` |
| `RADIO_NRESET` | `PA0` | SX1280 reset |
| `RADIO_BUSY` | `PB3` | SX1280 busy |
| `RADIO_DIO1` | `PB11` | `TIM2_CH4` input capture |
| `RADIO_DIO1_WAKEUP` | `PB4` | `EXTI4` wakeup input |
| `RADIO_ANTSEL` | `PA9` | antenna select |

Debug UART:

| Signal | MCU Pin | Description |
|---|---|---|
| `UART_TX` | `PA2` | `USART2_TX` via ST-Link VCP |
| `UART_RX` | `PA3` | `USART2_RX` via ST-Link VCP |

Recommended logic analyzer probes:

| Observation Target | MCU Pin | Description |
|---|---|---|
| `RADIO_TX_IND` | `PA11` | radio TX activity indicator |
| `RADIO_RX_IND` | `PA12` | radio RX activity indicator |
| `RADIO_NSS` | `PA8` | SPI transaction boundary |
| `RADIO_DIO1_WAKEUP` | `PB4` | observable DIO1 wakeup path |

## L476-Side SX1280 Wiring Table

| SX1280 Signal | STM32L476RG Pin | NUCLEO-L476RG Location | Notes |
|---|---|---|---|
| `NSS_CTS` | `PA8` | `CN5 pin 10 (D7)` / `CN10 pin 21` | chip select |
| `SCK_RTSN` | `PA5` | `CN5 pin 6 (D13)` | `SPI1_SCK` |
| `MISO_TX` | `PA6` | `CN5 pin 5 (D12)` | `SPI1_MISO` |
| `MOSI_RX` | `PA7` | `CN5 pin 4 (D11)` | `SPI1_MOSI` |
| `NRESET` | `PA0` | `CN8 pin 1 (A0)` / `CN7 pin 28` | reset |
| `BUSY` | `PB3` | `CN9 pin 4 (D3)` / `CN10 pin 31` | busy input |
| `DIO1` | `PB4` | `CN9 pin 6 (D5)` / `CN10 pin 27` | interrupt, jumper to `PB11` required |
| `DIO1 capture` | `PB11` | `CN10 pin 18` | `TIM2_CH4` capture input |
| `ANTSEL` | `PA9` | `CN5 pin 1 (D8)` / `CN10 pin 1` | antenna select |
| `GND` | `GND` | `CN5 pin 8` / `CN6 pin 6` | ground |
| `3.3V` | `3V3` | `CN6 pin 4` | power |
| `UART_TX` | `PA2` | `CN9 pin 2 (D1)` / `CN10 pin 35` | debug UART TX |
| `UART_RX` | `PA3` | `CN9 pin 1 (D0)` / `CN10 pin 37` | debug UART RX |

## DIO1 Jumper Explanation

The `SX1280 DIO1` output connects to `PB4 (D5)` on the NUCLEO board.

A jumper wire from `PB4` to `PB11` is required because:

- `PB4` is used as `EXTI4` wakeup (`RADIO_DIO1_WAKEUP`)
- `PB11` is used as `TIM2_CH4` capture input (`RADIO_DIO1`)

This keeps both the wakeup path and the timestamp capture path active.

## Comparison with Previous SX1262 Wiring

| Signal | SX1262 Connection | SX1280 Connection | MCU Pin | Change |
|---|---|---|---|---|
| NRESET | A0 | A0 | PA0 | none |
| BUSY | D3 | D3 | PB3 | none |
| DIO1 | PB11 (Morpho) | D5 (PB4) + jumper to PB11 | PB4/PB11 | route direction reversed |
| MISO | D12 | D12 | PA6 | none |
| MOSI | D11 | D11 | PA7 | none |
| SCK | D13 | D13 | PA5 | none |
| NSS | D7 | D7 | PA8 | none |
| ANT_SW / ANTSEL | D8 | D8 | PA9 | signal name only |
| Jumper | PB11 -> PB4 | PB4 -> PB11 | - | direction reversed |

All MCU pins remain the same.

The only physical difference is the DIO1 routing direction and the signal naming change from `ANT_SW` to `ANTSEL`.

## Common Hardware Notes

### 1. `SX1280 DIO1` is on `D5`, not directly on `PB11`

You must add a `PB4 -> PB11` jumper for `TIM2_CH4` input capture.

### 2. This setup still needs both Arduino and Morpho headers

`PB4` is on the Arduino side, while `PB11` is still the actual capture input on the Morpho side.

### 3. The `PB4 <-> PB11` jumper is mandatory

Without this jumper, radio interrupt timestamp capture will not work correctly.

### 4. `SX1280` uses `2.4 GHz`

This affects:

- radio frequency configuration
- antenna selection
- modulation and timing parameters

### 5. `PB3 / SWO` note

`RADIO_BUSY` is connected to `PB3`.

`PB3` is also `JTDO/TRACESWO` (AF0). The ST-LINK debugger sets `DBGMCU->CR` bit `TRACE_IOEN`, which survives soft resets and causes the CoreSight TPIU to drive `PB3` high. The firmware clears this bit and forces `PB3` to input mode before `MX_GPIO_Init()` in `Src/main.c`.

`SB15` (connecting `PB3` to ST-LINK SWO) does not need to be removed — the original `eval_l476` baseline works with `SB15` connected.

### 6. `NRESET` note on `DLP-RFS1280`

The `SX1280` chip datasheet says `NRESET` only has an internal `50 kOhm` pull-up, so in theory the `STM32` push-pull output on `PA0` should be able to drive it low without difficulty.

However, the current module-level behavior does not match the bare-chip expectation. A disconnect test showed:

- with the `DLP-RFS1280` module connected, software reads the reset line as staying high
- after disconnecting the module side, the same MCU output can be driven low normally

This strongly suggests the `DLP-RFS1280` module adds extra reset-side circuitry or a stronger pull-up path. The public `DLP-RFS1280` datasheet does not provide an internal schematic, so the exact cause cannot be confirmed from vendor documentation alone.

Current practical conclusion:

- normal radio operation is not blocked by this fact by itself
- software-controlled hard reset of the radio module cannot currently be assumed to work
- if the radio enters an abnormal state that requires a true hardware reset, a full power-cycle may be required

## Related Project Documents

- workflow and AI execution rules: `.ai/WORKFLOW.md`
- overall migration plan and module status: `.plan/MIGRATION_PLAN.md`
- module detailed specs: `.plan/`
- issue tracking and solved methods: `.ai/PORTING_REPORT.md`
- chronological change log: `.ai/CHANGELOG.md`
