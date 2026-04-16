/*!
 * \file      sx1280-board.h
 *
 * \brief     Target board SX1280 driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#ifndef __SX1280_BOARD_H__
#define __SX1280_BOARD_H__



#ifndef SX1280_CHECK_CMD_RETVAL
#define SX1280_CHECK_CMD_RETVAL   1         // set to 1 to check return values for sent commands
#endif /* SX1280_CHECK_CMD_RETVAL */

#ifndef SX1280_USE_ACCESS_LOCK
#define SX1280_USE_ACCESS_LOCK    1         // set to 1 to use a lock (semaphore) for the radio SPI access to prevent nesting
#endif /* SX1280_USE_ACCESS_LOCK */

#ifndef SX1280_PRINT_ERRORS
#define SX1280_PRINT_ERRORS       1         // by default print errors
#endif /* SX1280_PRINT_ERRORS */

#define SX1280_CMD_TIMEOUT        100       // timeout for sending a command to the radio, in HAL ticks

/*!
 * \brief Check for low-level radio command errors (write / read failures)
 *
 * \retval   errcnt     Returns the number of errors since the last reset
 */
uint32_t SX1280CheckCmdError( bool reset_counter );

/*!
 * \brief HW Reset of the radio
 */
void SX1280Reset( void );

/*!
 * \brief Blocking loop to wait while the Busy pin in high
 */
void SX1280WaitOnBusy( void );

/*!
 * \brief Wakes up the radio
 */
void SX1280Wakeup( void );

/*!
 * \brief Perform a minimal board-level communication probe.
 *
 * \retval                    Returns true on success
 */
bool SX1280Probe( void );

/*!
 * \brief Send a command that write data to the radio
 *
 * \param [in]  opcode        Opcode of the command
 * \param [in]  buffer        Buffer to be send to the radio
 * \param [in]  size          Size of the buffer to send
 *
 * \retval                    Returns true on success
 */
bool SX1280WriteCommand( RadioCommands_t opcode, uint8_t *buffer, uint16_t size );

bool SX1280WriteCommandWithoutExecute( RadioCommands_t command, uint8_t *buffer, uint16_t size);

/*!
 * \brief Send a command that read data from the radio
 *
 * \param [in]  opcode        Opcode of the command
 * \param [out] buffer        Buffer holding data from the radio
 * \param [in]  size          Size of the buffer
 *
 * \retval                    Returns true on success
 */
bool SX1280ReadCommand( RadioCommands_t opcode, uint8_t *buffer, uint16_t size );

/*!
 * \brief Write a single byte of data to the radio memory
 *
 * \param [in]  address       The address of the first byte to write in the radio
 * \param [in]  value         The data to be written in radio's memory
 *
 * \retval                    Returns true on success
 */
bool SX1280WriteRegister( uint16_t address, uint8_t value );

/*!
 * \brief Read a single byte of data from the radio memory
 *
 * \param [in]  address       The address of the first byte to write in the radio
 *
 * \retval      value         The value of the byte at the given address in radio's memory
 */
uint8_t SX1280ReadRegister( uint16_t address );

/*!
 * \brief Write data to the radio memory
 *
 * \param [in]  address       The address of the first byte to write in the radio
 * \param [in]  buffer        The data to be written in radio's memory
 * \param [in]  size          The number of bytes to write in radio's memory
 *
 * \retval                    Returns true on success
 */
bool SX1280WriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size );

/*!
 * \brief Read data from the radio memory
 *
 * \param [in]  address       The address of the first byte to read from the radio
 * \param [out] buffer        The buffer that holds data read from radio
 * \param [in]  size          The number of bytes to read from radio's memory
 *
 * \retval                    Returns true on success
 */
bool SX1280ReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size );

/*!
 * \brief Write data to the buffer holding the payload in the radio
 *
 * \param [in]  offset        The offset to start writing the payload
 * \param [in]  buffer        The data to be written (the payload)
 * \param [in]  size          The number of byte to be written
 *
 * \retval                    Returns true on success
 */
bool SX1280WriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );

/*!
 * \brief Read data from the buffer holding the payload in the radio
 *
 * \param [in]  offset        The offset to start reading the payload
 * \param [out] buffer        A pointer to a buffer holding the data from the radio
 * \param [in]  size          The number of byte to be read
 *
 * \retval                    Returns true on success
 */
bool SX1280ReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size );

/*!
 * \brief Sets the radio output power.
 *
 * \param [IN] power Sets the RF output power
 */
void SX1280SetRfTxPower( int8_t power );

/*!
 * \brief Initializes the RF Switch I/Os pins interface
 */
void SX1280AntSwOn( void );

/*!
 * \brief De-initializes the RF Switch I/Os pins interface
 *
 * \remark Needed to decrease the power consumption in MCU low power modes
 */
void SX1280AntSwOff( void );

#endif // __SX1280_BOARD_H__
