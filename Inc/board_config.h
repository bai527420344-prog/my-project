/*
 * Hardware board selection shared by the application, CMSIS and STM32 HAL.
 *
 * Select from the command line with:
 *   make BOARD_TYPE=1   # NUCLEO-L476 + DLP-RFS1280
 *   make BOARD_TYPE=0   # custom L476 + SX1280 ComBoard PCB
 */
#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

#define BOARD_CUSTOM_COMBOARD  0
#define BOARD_NUCLEO_L476      1

#ifndef BOARD_TYPE
#define BOARD_TYPE BOARD_NUCLEO_L476
#endif

#if (BOARD_TYPE != BOARD_CUSTOM_COMBOARD) && \
    (BOARD_TYPE != BOARD_NUCLEO_L476)
#error "Unsupported BOARD_TYPE (use 0 for custom PCB or 1 for Nucleo)"
#endif

/*
 * Both boards use a 48 MHz SYSCLK.  Keeping the same clock frequency also
 * keeps TIM2 at 8 MHz with its existing prescaler, which is required by the
 * LWB high-speed timer implementation.
 */
#define BOARD_SYSCLK_HZ       48000000U
#define BOARD_PLL_N           24U
#define BOARD_PLL_R_DIV       2U

#if BOARD_TYPE == BOARD_CUSTOM_COMBOARD

/* 12 MHz passive crystal connected to HOSC_IN/HOSC_OUT. */
#define BOARD_HSE_VALUE_HZ    12000000U
#define BOARD_HSE_IS_BYPASS   0
#define BOARD_PLL_M           3U

/* RF path is fixed to the external antenna; there is no ANTSEL net. */
#define BOARD_HAS_ANTSEL      0

/* PA13/PA14 are J400 SWDIO/SWCLK and must never be activity GPIOs. */
#define BOARD_HAS_PROG_GPIO   0

#else /* BOARD_NUCLEO_L476 */

/* 8 MHz ST-LINK MCO connected through SB16/SB50 to OSC_IN. */
#define BOARD_HSE_VALUE_HZ    8000000U
#define BOARD_HSE_IS_BYPASS   1
#define BOARD_PLL_M           2U

/* DLP-RFS1280 exposes ANTSEL on PA9 in the jumper-wire setup. */
#define BOARD_HAS_ANTSEL      1

/* PA12/PC4 are available as development-board activity GPIOs. */
#define BOARD_HAS_PROG_GPIO   1

#endif /* BOARD_TYPE */

#if ((BOARD_HSE_VALUE_HZ / BOARD_PLL_M) * BOARD_PLL_N / \
     BOARD_PLL_R_DIV) != BOARD_SYSCLK_HZ
#error "Board PLL constants do not produce BOARD_SYSCLK_HZ"
#endif

#endif /* BOARD_CONFIG_H_ */
