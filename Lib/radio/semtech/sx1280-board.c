/*
 * Copyright (c) 2018 - 2021, ETH Zurich, Computer Engineering Group (TEC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * based on sx1280dvk1cas-board.c by Semtech
 */

#include "flora_lib.h"


#if RADIO_ENABLE

#define SX1280_PROBE_BUSY_TIMEOUT_US               20000
#define SX1280_CMD_STATUS_VALID(status)     (((status) & 0x1C) < (0x4 << 2) || ((status) & 0x1C) > (0x5 << 2))  // cmdStatus in bits[4:2]; reject 4=error, 5=failure (datasheet p.95)



#if SX1280_PRINT_ERRORS

#define SX1280_ERROR(...)         sx1280_error_cnt++; LOG_ERROR(__VA_ARGS__)

#else /* SX1280_PRINT_ERRORS */

#define SX1280_ERROR(...)         sx1280_error_cnt++

#endif /* SX1280_PRINT_ERRORS */


#if SX1280_USE_ACCESS_LOCK

#ifndef SX1280AcquireLock         // if not used-defined, use the default lock implementation

semaphore_t sx1280_lock = 1;      // initial value 1 means this is a binary semaphore

#define SX1280AcquireLock()       if (!semaphore_acquire(&sx1280_lock)) \
                                  { \
                                      SX1280_ERROR("radio access denied"); \
                                      return false; \
                                  }
#define SX1280ReleaseLock()       semaphore_release(&sx1280_lock)

#endif /* SX1280AcquireLock */

#else /* SX1280_USE_ACCESS_LOCK */

// access lock not used -> define empty macros
#define SX1280AcquireLock()       1
#define SX1280ReleaseLock()

#endif /* SX1280_USE_ACCESS_LOCK */


static uint32_t sx1280_error_cnt = 0;


static bool SX1280WaitOnBusyWithTimeout( uint32_t timeout_us )
{
    if (RADIO_READ_NSS_PIN() == 0)
    {
        RADIO_SET_NSS_PIN();
        delay_us(1);
    }
    while( RADIO_READ_BUSY_PIN( ) )
    {
        if( timeout_us == 0 )
        {
            SX1280_ERROR("radio busy timeout");
            return false;
        }
        delay_us(1);
        timeout_us--;
    }
    return true;
}


uint32_t SX1280CheckCmdError( bool reset_counter )
{
    uint32_t cnt = sx1280_error_cnt;
    if (reset_counter)
    {
        sx1280_error_cnt = 0;
    }
    return cnt;
}

void SX1280Reset( void )
{
    delay_us(100);
    RADIO_CLR_NRESET_PIN();
    delay_us(200);
    RADIO_SET_NRESET_PIN();
    delay_us(100);

    // operating mode after reset is STDBY_RC
    SX1280SetOperatingMode(MODE_STDBY_RC);
}

void SX1280WaitOnBusy( void )
{
    if (RADIO_READ_NSS_PIN() == 0)
    {
        RADIO_SET_NSS_PIN();    // make sure the pin is high
        delay_us(1);            // 600ns max required between two NSS edges
    }
    uint32_t timeout = 10000;   // 10ms timeout (breadboard/jumper-wire tolerance)
    while( RADIO_READ_BUSY_PIN( ) )
    {
        if( timeout == 0 )
        {
            // NRESET not controllable on DLP-RFS1280 module —
            // force SetStandby(STDBY_RC) via SPI ignoring BUSY
            {
                uint8_t cmd  = 0x80;  // RADIO_SET_STANDBY
                uint8_t data = 0x00;  // STDBY_RC
                RADIO_CLR_NSS_PIN();
                delay_us(1);
                HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, 100);
                HAL_SPI_Transmit(&RADIO_SPI, &data, 1, 100);
                RADIO_SET_NSS_PIN();
                delay_us(100);
            }
            SX1280SetOperatingMode(MODE_STDBY_RC);
            // wait for BUSY to clear after forced standby
            timeout = 10000;
            while( RADIO_READ_BUSY_PIN( ) && timeout ) { delay_us(1); timeout--; }
            if( timeout == 0 )
            {
                SX1280_ERROR("radio unresponsive");
            }
            return;
        }
        delay_us(1);
        timeout--;
    }
}

void SX1280Wakeup( void )
{
    ENTER_CRITICAL_SECTION( );

    RADIO_CLR_NSS_PIN();      // falling edge will trigger the radio wake-up
    delay_us(100);
    SX1280WaitOnBusy( );

    SX1280SetOperatingMode(MODE_STDBY_RC);

    // make sure the antenna switch is turned on
    SX1280AntSwOn( );

    LEAVE_CRITICAL_SECTION( );
}

bool SX1280Probe( void )
{
    uint8_t firmwareVersion[2] = { 0 };
    uint8_t status = 0;

    RADIO_CLR_NRESET_PIN();
    delay_us(200);

    RADIO_SET_NRESET_PIN();
    delay_us(100);

    SX1280SetOperatingMode(MODE_STDBY_RC);
    if( !SX1280WaitOnBusyWithTimeout( SX1280_PROBE_BUSY_TIMEOUT_US ) )
    {
        return false;
    }

    if( !SX1280ReadCommand( RADIO_GET_STATUS, &status, 1 ) )
    {
        return false;
    }

    if( !SX1280ReadRegisters( REG_LR_FIRMWARE_VERSION_MSB, firmwareVersion, sizeof( firmwareVersion ) ) )
    {
        return false;
    }

    (void)status;
    (void)firmwareVersion;
    return true;
}

static bool SX1280SPIWrite( RadioCommands_t command, uint8_t *buffer, uint8_t size )
{
#if SX1280_CHECK_CMD_RETVAL

    uint8_t status = 0;
    // in case the command does not have any arguments, make sure the pointer is still valid and size is at least 1 to be able to read the status
    if (size == 0 || buffer == 0)
    {
      buffer = &status;
      size   = 1;
    }
    if (HAL_SPI_Transmit(&RADIO_SPI, (uint8_t*) &command, 1, SX1280_CMD_TIMEOUT)               != HAL_OK ||
        HAL_SPI_TransmitReceive(&RADIO_SPI, buffer, &status, 1, SX1280_CMD_TIMEOUT)            != HAL_OK ||
        !SX1280_CMD_STATUS_VALID(status)                                                                 ||
        ((size > 1) && (HAL_SPI_Transmit(&RADIO_SPI, &buffer[1], size - 1, SX1280_CMD_TIMEOUT) != HAL_OK)))
    {
        SX1280_ERROR("failed to send radio cmd (%x)", status);
        return false;
    }

#else /* SX1280_CHECK_CMD_RETVAL */

    HAL_SPI_Transmit(&RADIO_SPI, (uint8_t*) &command, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_Transmit(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT);

#endif /* SX1280_CHECK_CMD_RETVAL */

    return true;
}

bool SX1280WriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    bool success = true;

    SX1280AcquireLock( );

    SX1280CheckDeviceReady( );

    RADIO_CLR_NSS_PIN();
    delay_us(1);
    success = SX1280SPIWrite( command, buffer, size );
    RADIO_SET_NSS_PIN();
    delay_us(1);          // wait at least 600ns before continuing

    if( command != RADIO_SET_SLEEP )
    {
        SX1280WaitOnBusy( );
    }

    SX1280ReleaseLock();

    return success;
}

bool SX1280WriteCommandWithoutExecute( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    bool success = true;

    SX1280AcquireLock( );

    SX1280CheckDeviceReady( );

    RADIO_CLR_NSS_PIN();
    delay_us(1);
    success = SX1280SPIWrite( command, buffer, size );
    // no RADIO_SET_NSS_PIN(); as it will be timed precisely

    SX1280ReleaseLock();

    return success;
}

bool SX1280ReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    bool    success = true;
    uint8_t status  = 0;

    SX1280AcquireLock( );

    SX1280CheckDeviceReady( );

    RADIO_CLR_NSS_PIN();
    delay_us(1);

#if SX1280_CHECK_CMD_RETVAL

    if (HAL_SPI_Transmit(&RADIO_SPI, (uint8_t*) &command, 1, SX1280_CMD_TIMEOUT)      != HAL_OK ||
        HAL_SPI_TransmitReceive(&RADIO_SPI, &status, &status, 1, SX1280_CMD_TIMEOUT)  != HAL_OK ||
        !SX1280_CMD_STATUS_VALID(status)                                                        ||
        HAL_SPI_TransmitReceive(&RADIO_SPI, buffer, buffer, size, SX1280_CMD_TIMEOUT) != HAL_OK)
    {
        SX1280_ERROR("failed to execute read cmd (%x)", status);
        success = false;
    }

#else

    HAL_SPI_Transmit(&RADIO_SPI, (uint8_t*) &command, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_Transmit(&RADIO_SPI, (uint8_t*) &status, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_TransmitReceive(&RADIO_SPI, (uint8_t*) buffer, buffer, size, SX1280_CMD_TIMEOUT);

#endif /* SX1280_CHECK_CMD_RETVAL */

    RADIO_SET_NSS_PIN();
    delay_us(1);          // wait at least 600ns before continuing

    SX1280WaitOnBusy( );

    SX1280ReleaseLock();

    return success;
}

bool SX1280WriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    bool    success = true;
    uint8_t cmd     = RADIO_WRITE_REGISTER;
    uint8_t addr[2];

    addr[0] = address >> 8;    // MSB first!
    addr[1] = address & 0xff;

    SX1280AcquireLock( );

    SX1280CheckDeviceReady( );

    RADIO_CLR_NSS_PIN();
    delay_us(1);

#if SX1280_CHECK_CMD_RETVAL

    if (HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, 100)                     != HAL_OK ||
        HAL_SPI_Transmit(&RADIO_SPI, addr, 2, SX1280_CMD_TIMEOUT)      != HAL_OK ||
        HAL_SPI_Transmit(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT) != HAL_OK)
    {
        SX1280_ERROR("failed to write registers");
        success = false;
    }

#else /* SX1280_CHECK_CMD_RETVAL */

    HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, 100);
    HAL_SPI_Transmit(&RADIO_SPI, addr, 2, SX1280_CMD_TIMEOUT);
    HAL_SPI_Transmit(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT);

#endif /* SX1280_CHECK_CMD_RETVAL */

    RADIO_SET_NSS_PIN();
    delay_us(1);          // wait at least 600ns before continuing

    SX1280WaitOnBusy( );

    SX1280ReleaseLock();

    return success;
}

bool SX1280WriteRegister( uint16_t address, uint8_t value )
{
    return SX1280WriteRegisters( address, &value, 1 );
}

bool SX1280ReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    bool    success = true;
    uint8_t cmd     = RADIO_READ_REGISTER;
    uint8_t addr_ret[3];        // address and return value

    addr_ret[0] = address >> 8;     // MSB first!
    addr_ret[1] = address & 0xff;
    addr_ret[2] = 0;

    SX1280AcquireLock( );

    SX1280CheckDeviceReady( );

    RADIO_CLR_NSS_PIN();
    delay_us(1);

#if SX1280_CHECK_CMD_RETVAL

    if (HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, SX1280_CMD_TIMEOUT)     != HAL_OK ||
        HAL_SPI_Transmit(&RADIO_SPI, addr_ret, 3, SX1280_CMD_TIMEOUT) != HAL_OK ||
        !SX1280_CMD_STATUS_VALID(addr_ret[2])                                   ||
        HAL_SPI_Receive(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT) != HAL_OK)
    {
        SX1280_ERROR("failed to read registers");
        success = false;
    }

#else /* SX1280_CHECK_CMD_RETVAL */

    HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_Transmit(&RADIO_SPI, addr_ret, 3, SX1280_CMD_TIMEOUT);
    HAL_SPI_Receive(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT);

#endif /* SX1280_CHECK_CMD_RETVAL */

    RADIO_SET_NSS_PIN();
    delay_us(1);          // wait at least 600ns before continuing

    SX1280WaitOnBusy( );

    SX1280ReleaseLock();

    return success;
}

uint8_t SX1280ReadRegister( uint16_t address )
{
    uint8_t data = 0;
    SX1280ReadRegisters( address, &data, 1 );
    return data;
}

bool SX1280WriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    bool    success = true;
    uint8_t cmd     = RADIO_WRITE_BUFFER;

    SX1280AcquireLock( );

    SX1280CheckDeviceReady( );

    RADIO_CLR_NSS_PIN();
    delay_us(1);

#if SX1280_CHECK_CMD_RETVAL

    if (HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, SX1280_CMD_TIMEOUT)      != HAL_OK ||
        HAL_SPI_Transmit(&RADIO_SPI, &offset, 1, SX1280_CMD_TIMEOUT)   != HAL_OK ||
        HAL_SPI_Transmit(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT) != HAL_OK)
    {
        SX1280_ERROR("failed to write buffer");
        success = false;
    }

#else /* SX1280_CHECK_CMD_RETVAL */

    HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_Transmit(&RADIO_SPI, &offset, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_Transmit(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT);

#endif /* SX1280_CHECK_CMD_RETVAL */

    RADIO_SET_NSS_PIN();
    delay_us(1);          // wait at least 600ns before continuing

    SX1280WaitOnBusy( );

    SX1280ReleaseLock();

    return success;
}

bool SX1280ReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    bool    success = true;
    uint8_t cmd     = RADIO_READ_BUFFER;
    uint8_t status  = 0;

    SX1280AcquireLock( );

    SX1280CheckDeviceReady( );

    RADIO_CLR_NSS_PIN();
    delay_us(1);

#if SX1280_CHECK_CMD_RETVAL

    if (HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, SX1280_CMD_TIMEOUT)                    != HAL_OK ||
        HAL_SPI_Transmit(&RADIO_SPI, &offset, 1, SX1280_CMD_TIMEOUT)                 != HAL_OK ||
        HAL_SPI_TransmitReceive(&RADIO_SPI, &status, &status, 1, SX1280_CMD_TIMEOUT) != HAL_OK ||
        !SX1280_CMD_STATUS_VALID(status)                                                       ||
        HAL_SPI_Receive(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT)                != HAL_OK)
    {
        SX1280_ERROR("failed to read buffer");
        success = false;
    }

#else /* SX1280_CHECK_CMD_RETVAL */

    HAL_SPI_Transmit(&RADIO_SPI, &cmd, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_Transmit(&RADIO_SPI, &offset, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_Transmit(&RADIO_SPI, &status, 1, SX1280_CMD_TIMEOUT);
    HAL_SPI_Receive(&RADIO_SPI, buffer, size, SX1280_CMD_TIMEOUT);

#endif /* SX1280_CHECK_CMD_RETVAL */

    RADIO_SET_NSS_PIN();
    delay_us(1);          // wait at least 600ns before continuing

    SX1280WaitOnBusy( );

    SX1280ReleaseLock( );

    return success;
}

void SX1280SetRfTxPower( int8_t power )
{
    SX1280SetTxParams( power, RADIO_RAMP_20_US );
}

void SX1280AntSwOn( void )
{
#if BOARD_HAS_ANTSEL
    /* The jumper-wire development setup uses the DLP external antenna path. */
    HAL_GPIO_WritePin(RADIO_ANTSEL_GPIO_Port, RADIO_ANTSEL_Pin, GPIO_PIN_RESET);
#endif
}

void SX1280AntSwOff( void )
{
#if BOARD_HAS_ANTSEL
    /*
     * DLP-RFS1280 has a 100 kOhm pull-down on ANTSEL.  Driving ANTSEL high
     * while the radio sleeps wastes about 3.3 V / 100 kOhm = 33 uA.  The RF
     * switch has no true off state, so keep the externally selected antenna
     * state low between rounds and avoid that static current path.
     */
    HAL_GPIO_WritePin(RADIO_ANTSEL_GPIO_Port, RADIO_ANTSEL_Pin, GPIO_PIN_RESET);
#endif
}

#endif /* RADIO_ENABLE */
