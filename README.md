# LWB for NUCLEO-L433RC-P + SX1262 Evaluation Board

Low-Power Wireless Bus (LWB) application ported to run on the **NUCLEO-L433RC-P** development board with the **Semtech SX1262 Evaluation Board** (SX1262MB2CAS).

Originally designed for the DPP2 ComBoard. A how-to guide is available in the [Flora wiki](https://gitlab.ethz.ch/tec/public/flora/wiki#clone-compile-run).

## Hardware Setup

- **MCU Board**: ST NUCLEO-L433RC-P (STM32L433RCT6, Cortex-M4, 256KB Flash, 48KB SRAM)
- **Radio Board**: Semtech SX1262 Evaluation Board (SX1262MB2CAS)
- **Connection**: SX1262 eval board plugged into the Nucleo Arduino headers (CN5/CN9)

## Pin Mapping

### SX1262 Radio Interface (SPI2)

| Signal         | MCU Pin | Arduino Header | Direction |
|----------------|---------|----------------|-----------|
| RADIO_SCK      | PB13    | CN10 pin 30    | MCU -> SX1262 |
| RADIO_MISO     | PB14    | CN10 pin 28    | SX1262 -> MCU |
| RADIO_MOSI     | PB15    | CN10 pin 26    | MCU -> SX1262 |
| RADIO_NSS      | PC7     | CN5 pin 2 (D9) | MCU -> SX1262 |
| RADIO_NRESET   | PA0     | CN9 pin 1 (A0) | MCU -> SX1262 |
| RADIO_BUSY     | PB3     | CN9 pin 4 (D3) | SX1262 -> MCU |
| RADIO_DIO1     | PA15    | CN7 pin 17     | SX1262 -> MCU |
| RADIO_ANT_SW   | PB6     | CN5 pin 3 (D10)| MCU -> SX1262 |

### UART (ST-Link VCP, USART2)

| Signal   | MCU Pin | Note |
|----------|---------|------|
| UART_TX  | PA2     | Connected to ST-Link VCP via SB13 |
| UART_RX  | PA3     | Connected to ST-Link VCP via SB14 |

Serial config: **115200 baud, 8N1**. Device: `/dev/ttyACM0` (Linux).

### Debug / Indicator Pins

| Signal          | MCU Pin | Morpho Header | Description |
|-----------------|---------|---------------|-------------|
| TX indicator    | PA11    | CN10 pin 14   | High during radio TX (COM_GPIO2 in `main.h`, toggled by `RADIO_TX_START/STOP_IND()` in `app_config.h`) |
| RX indicator    | PA12    | CN10 pin 12   | High during radio RX (COM_PROG2 in `main.h`, toggled by `RADIO_RX_START/STOP_IND()` in `app_config.h`) |
| LED_GREEN       | PB8     | CN5 pin 6 (D6)| System LED |
| LED_RED         | PB9     | CN5 pin 5 (D5)| Event LED  |

### Clock Configuration

| Source | Frequency | Usage |
|--------|-----------|-------|
| HSI    | 16 MHz    | PLL input (PLLM=2, PLLN=8, PLLR=2 -> SYSCLK = 32 MHz) |
| LSE    | 32.768 kHz| LPTIM1 (low-power timer for LWB scheduling) |

Note: HSE is **not available** on the NUCLEO-L433RC-P board. The original comboard design used an 8 MHz HSE crystal; this port uses the internal HSI oscillator instead.

## Build and Flash

```bash
# Build
make -j4

# Flash (requires st-flash, ST-Link must be connected)
st-flash --connect-under-reset write build/comboard_lwb.bin 0x08000000

# Monitor serial output
stty -F /dev/ttyACM0 115200 cs8 -cstopb -parenb raw -echo && cat /dev/ttyACM0
```

## LWB Configuration

Key parameters in `Inc/app_config.h`:

| Parameter | Value | Description |
|-----------|-------|-------------|
| NODE_ID / HOST_ID | 2 | This node acts as the LWB host |
| LWB_SCHED_PERIOD | 15 s | Communication round period |
| LWB_N_TX | 2 | Number of retransmissions |
| LWB_NUM_HOPS | 6 | Max network hops |
| GLORIA_INTERFACE_MODULATION | 10 | FSK 250 kbit/s |
| GLORIA_INTERFACE_RF_BAND | 48 | 869.46 MHz |
| LOW_POWER_MODE | LP_MODE_STOP2 | Deep sleep between rounds |
