# Library Overview

The Flora library provides drivers and helper functions for the NUCLEO-L476RG MCU board with the Semtech SX1280 (2.4 GHz) radio module (DLP-RFS1280). Originally derived from the DPP2 LoRa comboard / STM32L433CC + SX1262 codebase; the L433→L476 and SX1262→SX1280 migrations are complete and the three SX1280 modulations (LoRa SF5, GFSK 125k, FLRC 260k) are all validated end-to-end on LWB.

Further information and a list of example apps are available in the [Flora wiki page](https://gitlab.ethz.ch/tec/public/flora/wiki).

## Folders

| Name          | Description                                                                                     |
|---------------|-------------------------------------------------------------------------------------------------|
| `bolt/`       | Contains Bolt files and its functions                                                           |
| `cli/`        | Command line interface (incl. `gloria`, `radio`, `develop`, `gfsk_test`, `flrc_test` commands)  |
| `deployment/` | Deployment-specific code                                                                        |
| `dpp/`        | Submodule, contains message definitions and some library functions (CRC, data structures, ...). |
| `flocklab/`   | Defines all FlockLab pins, provides functions to set and clear the FlockLab LEDs                |
| `protocol/`   | Wireless protocols (Gloria flooding, LWB scheduler)                                             |
| `radio/`      | Radio driver and helper functions for the Semtech SX1280 radio (2.4 GHz, LoRa/GFSK/FLRC).       |
| `system/`     | General system initialization and helper functions (UART, GPIO, ...).                           |
| `time/`       | Timers (LPTIM, TIM, RTC)                                                                        |
| `utils/`      | Misc helper functions / utilities (LEDs, logging, etc.)                                         |
