# LWB for NUCLEO-L476RG + SX1262 Evaluation Board

This repository was originally based on an `L433 + SX1262` project. The current `README` focuses on one thing first:
clarifying how the `SX1262` should be wired to the `NUCLEO-L476RG`, and what hardware preparation is required on the `L476` board itself.

Notes:
- The `STM32` pin definitions below follow the current project source in [`Inc/main.h`](Inc/main.h).
- The `NUCLEO-L476RG` header locations are based on the `UM1724 / nucleo-l476rg` documentation you provided.
- This file also references the L476 clock preparation notes in `/home/rosy-bxianrui/development-fork/flora_firsttry/README.md`, but the `CN10/18 <-> CN10/27` jumper mentioned there is specific to the `SX126xDVK1xAS DevKit` and should not be copied blindly to the `SX1262MB2CAS` evaluation board used here.
- Some original `L433` file names and configuration traces may still exist in this repository. This README currently updates the `L476` hardware wiring guidance first; it does not imply that the entire codebase migration to `L476` is already complete.

## Current Goal

The current goal is to replace the old `L433 + SX1262` hardware notes with an `L476 + SX1262` version.  
This document therefore focuses on two things:

1. The board-level preparation required on the `NUCLEO-L476RG`.
2. Which `L476` pins each `SX1262` signal should connect to under the current firmware definitions.

## NUCLEO-L476RG Board Preparation

The `L476` clock baseline that has already been validated on hardware in this repository is:

- `SYSCLK` uses `HSE_BYPASS + PLL`
- `RTC / LSE` uses `LSE`
- `USART2` uses `HSI`

So for the current bring-up code:

- Keep the solder bridge changes you already made: `SB55 OFF`, `SB54 ON`, `SB16 ON`, `SB50 ON`
- In other words, route `ST-LINK MCO 8 MHz` to the target MCU `HSE_BYPASS`
- This is not the default `NUCLEO-L476RG` board configuration

Additional clarification:

- `UM1724` indeed describes the default `NUCLEO-L476RG` as `HSE not used`
- But this repository no longer uses that default setup; it has been switched to the validated path `ST-LINK MCO 8 MHz -> HSE_BYPASS -> PLL`
- So this README follows the actual project code and validated board behavior, not the default NUCLEO clock description

## Current SX1262 Signal Definitions Used by the Firmware

The radio-related pins currently defined in [`Inc/main.h`](Inc/main.h) are:

| Signal | MCU Pin | Description |
|---|---|---|
| `RADIO_NSS` | `PA8` | SPI chip select |
| `RADIO_SCK` | `PA5` | SPI clock |
| `RADIO_MISO` | `PA6` | SPI MISO |
| `RADIO_MOSI` | `PA7` | SPI MOSI |
| `RADIO_NRESET` | `PA0` | SX1262 reset |
| `RADIO_BUSY` | `PB3` | SX1262 Busy |
| `RADIO_DIO1` | `PB11` | SX1262 interrupt/event input, connected to `TIM2_CH4` |
| `RADIO_DIO1_WAKEUP` | `PB4` | EXTI wakeup pin after the `PB11 -> PB4` jumper wire |
| `RADIO_ANT_SW` | `PA9` | antenna switch control |

Debug UART:

| Signal | MCU Pin | Description |
|---|---|---|
| `UART_TX` | `PA2` | `USART2_TX` via ST-Link VCP |
| `UART_RX` | `PA3` | `USART2_RX` via ST-Link VCP |

Recommended logic analyzer probes:

| Observation Target | MCU Pin | Description |
|---|---|---|
| `RADIO_TX_IND` | `PA11` | `COM_GPIO2`, used by the firmware to indicate radio `TX` activity |
| `RADIO_RX_IND` | `PA12` | `COM_PROG2`, used by the firmware to indicate radio `RX` activity |
| `RADIO_NSS` | `PA8` | SPI chip select, useful for aligning transaction boundaries |
| `RADIO_DIO1_WAKEUP` | `PB4` | observable wakeup pin after the `DIO1` jumper path |

In particular:

- `PA11` / `PA12` come from `RADIO_TX_START_IND()` / `RADIO_RX_START_IND()`
- If you only want to observe round cadence first, probe `PA11 + PA12`
- If you want finer alignment between SPI transactions and interrupt events, also add `PA8 + PB4`

## L476-Side Wiring Table

The table below lists only the `NUCLEO-L476RG` side, so you can wire by signal name instead of reusing old `L433` connector numbers directly.

| SX1262 Signal | STM32L476RG Pin | NUCLEO-L476RG Location | Notes |
|---|---|---|---|
| `NSS` | `PA8` | `CN5 pin 10 (D7)` / `CN10 pin 21` | chip select |
| `SCK` | `PA5` | `CN5 pin 6 (D13)` | `SPI1_SCK` |
| `MISO` | `PA6` | `CN5 pin 5 (D12)` | `SPI1_MISO` |
| `MOSI` | `PA7` | `CN5 pin 4 (D11)` | `SPI1_MOSI` |
| `NRESET` | `PA0` | `CN8 pin 1 (A0)` / `CN7 pin 28` | reset |
| `BUSY` | `PB3` | `CN9 pin 4 (D3)` / `CN10 pin 31` | Busy input |
| `DIO1` | `PB11` | `CN10 pin 18` | `TIM2_CH4` input capture |
| `DIO1_WAKEUP` | `PB4` | `CN9 pin 6 (D5)` / `CN10 pin 27` | requires the `PB11 -> PB4` jumper wire |
| `ANT_SW` | `PA9` | `CN5 pin 1 (D8)` / `CN10 pin 1` | antenna switch |
| `UART_TX` | `PA2` | `CN9 pin 2 (D1)` / `CN10 pin 35` | ST-Link virtual COM TX |
| `UART_RX` | `PA3` | `CN9 pin 1 (D0)` / `CN10 pin 37` | ST-Link virtual COM RX |

## SX1262 J1 Mapping Confirmed by You

The latest board-side information you confirmed is:

- `J1-1 -> D0`
- `J1-2 -> D1`
- `J1-3 -> D2`
- `J1-4 -> D3`
- `J1-5 -> D4`
- `J1-6 -> D5`
- `J1-7 -> D6`
- `J1-8 -> D7`

Mapping that onto the `NUCLEO-L476RG` `CN9` definitions gives:

| SX1262 J1 pin | Arduino Signal | NUCLEO-L476RG Pin |
|---|---|---|
| `J1-1` | `D0` | `PA3` |
| `J1-2` | `D1` | `PA2` |
| `J1-3` | `D2` | `PA10` |
| `J1-4` | `D3` | `PB3` |
| `J1-5` | `D4` | `PB5` |
| `J1-6` | `D5` | `PB4` |
| `J1-7` | `D6` | `PB10` |
| `J1-8` | `D7` | `PA8` |

## What the J1 Mapping Means for the Current Firmware

This `J1 -> D0~D7` information is important because it shows that the `J1` area on the `L476` does not automatically expose every pin needed by the current firmware.

Directly compared against the current firmware:

- `J1-4 -> D3 -> PB3` matches `RADIO_BUSY = PB3`
- `J1-6 -> D5 -> PB4` can serve as `RADIO_DIO1_WAKEUP = PB4`
- `J1-8 -> D7 -> PA8` matches `RADIO_NSS = PA8`
- `J1` still does not expose `PB11`, which is the actual timestamp capture input, so the `PB11 -> PB4` jumper is still required

In other words:

- `J1` can directly provide `BUSY`
- `J1-8` can directly provide `NSS`
- `J1-6` can directly provide `PB4` for `DIO1_WAKEUP`
- But the real `DIO1 capture = PB11` still needs the extra jumper wire

## SX1262 J2 Mapping Confirmed by You

The latest board-side information you confirmed is:

- `J2-1 -> D8`
- `J2-2 -> D9`
- `J2-3 -> D10`
- `J2-4 -> D11`
- `J2-5 -> D12`
- `J2-6 -> D13`
- `J2-7 -> GND`
- `J2-8 -> AVDD`
- `J2-9 -> D14`
- `J2-10 -> D15`

Mapping that onto the `NUCLEO-L476RG` `CN5` definitions gives:

| SX1262 J2 pin | Arduino Signal | NUCLEO-L476RG Pin |
|---|---|---|
| `J2-1` | `D8` | `PA9` |
| `J2-2` | `D9` | `PC7` |
| `J2-3` | `D10` | `PB6` |
| `J2-4` | `D11` | `PA7` |
| `J2-5` | `D12` | `PA6` |
| `J2-6` | `D13` | `PA5` |
| `J2-7` | `GND` | `GND` |
| `J2-8` | `AVDD` | `AVDD` |
| `J2-9` | `D14` | `PB9` |
| `J2-10` | `D15` | `PB8` |

## What the J2 Mapping Means for the Current Firmware

This `J2` information is also valuable because it already aligns one key control signal and the `SPI1` lines used by the current firmware:

- `J2-1 -> D8 -> PA9` matches `RADIO_ANT_SW = PA9`
- The physical `J2-2 -> D9 -> PC7` mapping still exists, but the current firmware no longer uses it as `NSS`

Also, the current code has already moved to `SPI1` with this mapping:

- `J2-4 -> D11 -> PA7 -> RADIO_MOSI`
- `J2-5 -> D12 -> PA6 -> RADIO_MISO`
- `J2-6 -> D13 -> PA5 -> RADIO_SCK`

This means we can now confirm:

- `J1` can cover `BUSY`
- `J1` can cover `NSS`
- `J2` can cover `ANT_SW`
- `J2` also aligns all three `SPI1` lines in one shot
- The main remaining extra wiring is still the `DIO1 = PB11` and `DIO1_WAKEUP = PB4` jumper relationship

## SX1262 J3 Mapping Confirmed by You

The latest board-side information you confirmed is:

- `J3-1 -> NC`
- `J3-2 -> IOREF`
- `J3-3 -> RESET`
- `J3-4 -> 3V3`
- `J3-5 -> 5V`
- `J3-6 -> GND`
- `J3-7 -> GND`
- `J3-8 -> VIN`

Mapping that onto the `NUCLEO-L476RG` `CN6/CN8` power definitions gives:

| SX1262 J3 pin | Arduino Signal | NUCLEO-L476RG Location |
|---|---|---|
| `J3-1` | `NC` | `NC` |
| `J3-2` | `IOREF` | `CN6 pin 2` |
| `J3-3` | `RESET` | `CN6 pin 3 (NRST)` |
| `J3-4` | `3V3` | `CN6 pin 4 (+3V3)` |
| `J3-5` | `5V` | `CN6 pin 5 (+5V)` |
| `J3-6` | `GND` | `CN6 pin 6 (GND)` |
| `J3-7` | `GND` | `CN6 pin 7 (GND)` |
| `J3-8` | `VIN` | `CN6 pin 8 (VIN)` |

## What the J3 Mapping Means for the Current Firmware

`J3` mainly describes power and board-level reset. For the current firmware, it leads to two conclusions:

- `J3-3 -> RESET` is the board `NRST` for the whole `NUCLEO-L476RG`, not the firmware signal `RADIO_NRESET = PA0`
- So the required `SX1262 NRESET` must still be wired separately to `PA0`; `J3-3` cannot replace it

Also, `J3` confirms that this area is just the standard Arduino power section:

- `3V3`
- `5V`
- `VIN`
- `GND`

This means `J3` currently does not provide any extra `SPI` or `DIO1` wiring capability.

## SX1262 J4 Mapping Confirmed by You

The latest board-side information you confirmed is:

- `J4-1 -> A0`
- `J4-2 -> A1`
- `J4-3 -> A2`
- `J4-4 -> A3`
- `J4-5 -> A4`
- `J4-6 -> A5`

Mapping that onto the `NUCLEO-L476RG` `CN8` analog header definitions gives:

| SX1262 J4 pin | Arduino Signal | NUCLEO-L476RG Pin |
|---|---|---|
| `J4-1` | `A0` | `PA0` |
| `J4-2` | `A1` | `PA1` |
| `J4-3` | `A2` | `PA4` |
| `J4-4` | `A3` | `PB0` |
| `J4-5` | `A4` | `PC1` or `PB9` |
| `J4-6` | `A5` | `PC0` or `PB8` |

Additional note:

- `A4/A5` on the `NUCLEO-L476RG` have on-board multiplexing; `UM1724` lists them as `PC1 or PB9` and `PC0 or PB8`
- So if you later want to use `J4-5/J4-6` as explicit GPIOs, you will also need to confirm the related bridge configuration on the board

## What the J4 Mapping Means for the Current Firmware

`J4` finally lines up one key firmware pin directly:

- `J4-1 -> A0 -> PA0` matches `RADIO_NRESET = PA0`

But `J4` also shows:

- `J4` does not provide the required `DIO1 = PB11`
- `J4` is not involved in the current firmware `SPI1 = PA5/PA6/PA7`

So, combining the `J1/J2/J3/J4` information you already confirmed, we can conclude:

- `BUSY` can be satisfied by `J1-4 -> D3 -> PB3`
- `NSS` can be satisfied by `J1-8 -> D7 -> PA8`
- `ANT_SW` can be satisfied by `J2-1 -> D8 -> PA9`
- `NRESET` can be satisfied by `J4-1 -> A0 -> PA0`
- The three SPI lines are Arduino `D11/D12/D13 = PA7/PA6/PA5`
- `DIO1_WAKEUP` can use `J1-6 -> D5 -> PB4`
- The one signal that still needs an extra jumper is `DIO1 capture = PB11`

## Common Wiring Pitfalls

### 1. Do Not Copy the L433 Header Numbers Directly

Although many signal names are the same, the `Arduino / Morpho` mapping is not identical between the `NUCLEO-L476RG` and the `NUCLEO-L433RC-P`.  
In this `L476` migration, `SPI` has already moved back to the standard Arduino `D11/D12/D13`, but `PB11` for `DIO1 capture` is still on the `Morpho` header.

So during migration, wire by `signal name -> MCU pin -> L476 header location`, not by mechanically reusing the old README `CNx/Jx` numbering.

### 2. This Setup Cannot Be Completed Using Only the Arduino-Compatible Headers

From the tables above:

- `NSS`, `BUSY`, `NRESET`, `ANT_SW`, and the three `SPI1` lines can all be found on the Arduino-related headers
- But the actual timestamp capture input `DIO1 = PB11` still requires the `ST morpho` header

That means if your `SX1262` board is only plugged in like a normal Arduino shield, it is still not enough; you will usually need extra jumpers for:

- `PB11` (`CN10 pin 18`) as `RADIO_DIO1`
- `PB4` (`CN10 pin 27` / `CN9 pin 6`) as `RADIO_DIO1_WAKEUP`

In other words, the jumper you already mentioned:

- `PB11 -> PB4`

This conclusion comes from:

- the official `L476` header definitions
- the actual `main.h` pin definitions in the current project

### 3. This Migration Explicitly Depends on One DIO1 Jumper Wire

The code in this repository now clearly implements the following relationship:

- `PB11 = RADIO_DIO1`
- `PB4 = RADIO_DIO1_WAKEUP`
- `PB11 -> PB4` jumper present

So this jumper is no longer just something “for reference in the old project”; it is part of the actual bring-up solution used by the current `eval_l476` project.

## Minimal Wiring Checklist

If you only want to get the `SX1262` connected to the `L476` first, make sure at least the following signals are correct:

- `NSS -> PA8`
- `SCK -> PA5`
- `MISO -> PA6`
- `MOSI -> PA7`
- `NRESET -> PA0`
- `BUSY -> PB3`
- `DIO1 -> PB11`
- `DIO1_WAKEUP -> PB4`
- `PB11 -> PB4` jumper wire
- `ANT_SW -> PA9`
- `GND -> GND`
- connect `PA2/PA3` to `USART2 / ST-Link VCP` if you want to observe logs over UART

## Build and Serial

```bash
make -j4

st-flash --reset --connect-under-reset write build/comboard_lwb.bin 0x08000000

stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb raw -echo && cat /dev/ttyACM0
```

Serial settings:

- `115200`
- `8N1`
- on Linux, the device is usually `/dev/ttyACM0`

Notes:
- Only the build/flash commands that already exist in this repository are kept here, so ongoing bring-up and validation stay simple.
- If you switch the `NODE_ID / HOST_ID` role configuration in `Inc/app_config.h`, run `make clean` before `make -j4` to avoid stale object files causing the board to run a different role than the source code suggests.
- For this board, flashing with `--reset` is recommended so the MCU restarts more reliably after programming; in most cases you should not need to press the board `NRST` manually.
- If hterm still does not print immediately, a manual `reset` can still be used as a fallback.
- The current stable baseline has already been validated in a `host + source` dual-node setup, and after increasing the `SPI1` clock the `Schedule too late!` warning disappeared in the dual-node scenario.
- The current stable baseline parameters are:
  - `SPI1` prescaler `SPI_BAUDRATEPRESCALER_4`
  - `LOG_LEVEL = LOG_LEVEL_INFO`
  - `host + source` dual-node validation passed
- If the project is later fully switched over to `STM32L476RG`, the startup files, linker script, CubeMX project, and chip-model-specific configuration will still need to be kept in sync.

## Future Additions

Once you provide the `SX1262` board-side connector diagram, photos, or the `J1/J2/J3/J4` mapping table, this README can be extended into a more complete version with:

- `SX1262` board-side connector numbering
- `SX1262` board-side power pin mapping
- whether any extra jumpers are required
- one final complete board-to-board wiring table
