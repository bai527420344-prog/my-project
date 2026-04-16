/*!
 * \file      sx1280.c
 *
 * \brief     SX1280 driver implementation
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

/*
 * reference implementation is available at https://github.com/Lora-net/LoRaMac-node/blob/master/src/radio/sx1280/sx1280.c
 */

#include "flora_lib.h"

#if RADIO_ENABLE

/*!
 * \brief Internal frequency of the radio
 */
#define SX1280_XTAL_FREQ                            52000000UL

/*!
 * \brief Scaling factor used to perform fixed-point operations
 *
 * SX1280 PLL step = XTAL / 2^18  (NOT 2^25 as on SX126x).
 * SHIFT_AMOUNT must be small enough so that
 *   (PLL_STEP_SCALED - 1) << SHIFT_AMOUNT  fits in uint32_t.
 * With SHIFT = 12 the worst-case intermediate is ~3.3 G, which fits.
 */
#define SX1280_PLL_STEP_SHIFT_AMOUNT                ( 12 )

/*!
 * \brief PLL step - scaled with SX1280_PLL_STEP_SHIFT_AMOUNT
 */
#define SX1280_PLL_STEP_SCALED                      ( SX1280_XTAL_FREQ >> ( 18 - SX1280_PLL_STEP_SHIFT_AMOUNT ) )

/*!
 * \brief Radio registers definition
 */
typedef struct
{
    uint16_t      Addr;                             //!< The address of the register
    uint8_t       Value;                            //!< The value of the register
}RadioRegisters_t;

/*!
 * \brief Holds the internal operating mode of the radio
 */
static RadioOperatingModes_t OperatingMode;

/*!
 * \brief Stores the current packet type set in the radio
 */
static RadioPacketTypes_t PacketType;

/*!
 * \brief Get the number of PLL steps for a given frequency in Hertz
 *
 * \param [in] freqInHz Frequency in Hertz
 *
 * \returns Number of PLL steps
 */
static uint32_t SX1280ConvertFreqInHzToPllStep( uint32_t freqInHz );
/*
 * SX1280 DIO IRQ callback functions prototype
 */

/*!
 * \brief DIO 0 IRQ callback
 */
void SX1280OnDioIrq( void );

/*!
 * \brief DIO 0 IRQ callback
 */
void SX1280SetPollingMode( void );

/*!
 * \brief DIO 0 IRQ callback
 */
void SX1280SetInterruptMode( void );

/*
 * \brief Process the IRQ if handled by the driver
 */
void SX1280ProcessIrqs( void );

/*
 * External helper functions
 */

void SX1280Init( )
{
    SX1280Reset( );

    /* DLP-RFS1280 keeps NRESET effectively high, so after an MCU reset the
     * radio may still be asleep and BUSY may remain asserted. On this module
     * an NSS-only wakeup toggle is unreliable, so send a raw SetStandby
     * command once to recover STDBY_RC when BUSY is already high. */
    if( RADIO_READ_BUSY_PIN( ) )
    {
        /* Force STDBY_RC via raw SPI, bypassing the BUSY gate */
        uint8_t cmd[2]   = { 0x80, 0x00 };   /* SET_STANDBY, STDBY_RC */
        uint8_t dummy[2];

        RADIO_CLR_NSS_PIN( );
        delay_us( 10 );
        HAL_SPI_TransmitReceive( &hspi1, cmd, dummy, 2, 100 );
        RADIO_SET_NSS_PIN( );

        /* Wait up to 5 ms for the radio to reach STDBY_RC */
        for( int i = 0; i < 50; i++ )
        {
            delay_us( 100 );
            if( !RADIO_READ_BUSY_PIN( ) ) break;
        }
    }

    SX1280WaitOnBusy( );
    SX1280SetStandby( STDBY_RC );
}

RadioOperatingModes_t SX1280GetOperatingMode( void )
{
    return OperatingMode;
}

void SX1280SetOperatingMode( RadioOperatingModes_t mode )
{
    OperatingMode = mode;
}

void SX1280CheckDeviceReady( void )
{
    if( ( SX1280GetOperatingMode( ) == MODE_SLEEP ) || ( SX1280GetOperatingMode( ) == MODE_RX_DC ) )
    {
        SX1280Wakeup( );
    }
    SX1280WaitOnBusy( );
}

void SX1280SetPayload( uint8_t *payload, uint8_t size )
{
    SX1280WriteBuffer( 0x00, payload, size );
}

uint8_t SX1280GetPayload( uint8_t *buffer, uint8_t *size,  uint8_t maxSize )
{
    uint8_t offset = 0;

    SX1280GetRxBufferStatus( size, &offset );
    if( *size > maxSize )
    {
        return 1;
    }
    SX1280ReadBuffer( offset, buffer, *size );
    return 0;
}

void SX1280SendPayload( uint8_t *payload, uint8_t size, uint32_t timeout )
{
    SX1280SetPayload( payload, size );
    SX1280SetTx( timeout, true );
}

uint8_t SX1280SetSyncWord( uint8_t *syncWord )
{
    SX1280WriteRegisters( REG_LR_SYNCWORDBASEADDRESS, syncWord, 8 );
    return 0;
}

void SX1280SetCrcSeed( uint16_t seed )
{
    uint8_t buf[2];

    buf[0] = ( uint8_t )( ( seed >> 8 ) & 0xFF );
    buf[1] = ( uint8_t )( seed & 0xFF );

    switch( SX1280GetPacketType( ) )
    {
        case PACKET_TYPE_GFSK:
            SX1280WriteRegisters( REG_LR_CRCSEEDBASEADDR, buf, 2 );
            break;

        default:
            break;
    }
}

void SX1280SetCrcPolynomial( uint16_t polynomial )
{
    uint8_t buf[2];

    buf[0] = ( uint8_t )( ( polynomial >> 8 ) & 0xFF );
    buf[1] = ( uint8_t )( polynomial & 0xFF );

    switch( SX1280GetPacketType( ) )
    {
        case PACKET_TYPE_GFSK:
            SX1280WriteRegisters( REG_LR_CRCPOLYBASEADDR, buf, 2 );
            break;

        default:
            break;
    }
}

void SX1280SetWhiteningSeed( uint16_t seed )
{
    uint8_t regValue = 0;

    switch( SX1280GetPacketType( ) )
    {
        case PACKET_TYPE_GFSK:
            regValue = SX1280ReadRegister( REG_LR_WHITSEEDBASEADDR_MSB ) & 0xFE;
            regValue = ( ( seed >> 8 ) & 0x01 ) | regValue;
            SX1280WriteRegister( REG_LR_WHITSEEDBASEADDR_MSB, regValue ); // only 1 bit.
            SX1280WriteRegister( REG_LR_WHITSEEDBASEADDR_LSB, ( uint8_t )seed );
            break;

        default:
            break;
    }
}

uint32_t SX1280GetRandom( void )
{
    uint32_t number = 0;

    // Set radio in continuous reception
    SX1280SetRx( 0xFFFFFF, true, false ); // Rx Continuous

    SX1280ReadRegisters( RANDOM_NUMBER_GENERATORBASEADDR, ( uint8_t* )&number, 4 );

    SX1280SetStandby( STDBY_RC );

    return number;
}

void SX1280SetSleep( SleepParams_t sleepConfig )
{
    if( SX1280WriteCommand( RADIO_SET_SLEEP, &sleepConfig.Value, 1 ) )
    {
        SX1280AntSwOff( );

        OperatingMode = MODE_SLEEP;
    }
}

void SX1280SetStandby( RadioStandbyModes_t standbyConfig )
{
    if( SX1280WriteCommand( RADIO_SET_STANDBY, ( uint8_t* )&standbyConfig, 1 ) )
    {
        if( standbyConfig == STDBY_RC )
        {
            OperatingMode = MODE_STDBY_RC;
        }
        else
        {
            OperatingMode = MODE_STDBY_XOSC;
        }
    }
}

void SX1280SetFs( void )
{
    if( SX1280WriteCommand( RADIO_SET_FS, 0, 0 ) )
    {
        OperatingMode = MODE_FS;
    }
}

void SX1280SetTx( uint32_t timeout, bool execute )
{
    uint8_t buf[3];
    bool    success;

    buf[0] = ( uint8_t )( ( timeout >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( timeout >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( timeout & 0xFF );

    if( execute )
    {
        success = SX1280WriteCommand( RADIO_SET_TX, buf, 3 );
    }
    else
    {
        success = SX1280WriteCommandWithoutExecute( RADIO_SET_TX, buf, 3 );
    }
    if( success )
    {
        OperatingMode = MODE_TX;
    }
}

void SX1280SetRx( uint32_t timeout, bool execute, bool boosted )
{
    uint8_t buf[3];
    bool    success;

    if( boosted )
    {
        SX1280WriteRegister( REG_RX_GAIN, 0x96 ); // max LNA gain, increase current by ~2mA for around ~3dB in sensitivity
    }

    buf[0] = ( uint8_t )( ( timeout >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( timeout >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( timeout & 0xFF );

    if( execute )
    {
        success = SX1280WriteCommand( RADIO_SET_RX, buf, 3 );
    }
    else
    {
        success = SX1280WriteCommandWithoutExecute( RADIO_SET_RX, buf, 3 );
    }
    if( success )
    {
        if( timeout == 0xFFFFFF )
        {
            OperatingMode = MODE_RX_CONTINUOUS;
        }
        else
        {
            OperatingMode = MODE_RX;
        }
    }
}

void SX1280SetRxDutyCycle( uint32_t rxTime, uint32_t sleepTime, bool execute )
{
    uint8_t buf[6];
    bool    success;

    buf[0] = ( uint8_t )( ( rxTime >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( rxTime >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( rxTime & 0xFF );
    buf[3] = ( uint8_t )( ( sleepTime >> 16 ) & 0xFF );
    buf[4] = ( uint8_t )( ( sleepTime >> 8 ) & 0xFF );
    buf[5] = ( uint8_t )( sleepTime & 0xFF );

    if( execute )
    {
        success = SX1280WriteCommand( RADIO_SET_RXDUTYCYCLE, buf, 6 );
    }
    else
    {
        success = SX1280WriteCommandWithoutExecute( RADIO_SET_RXDUTYCYCLE, buf, 6 );
    }
    if( success )
    {
        OperatingMode = MODE_RX_DC;
    }
}

void SX1280SetCad( void )
{
    if( SX1280WriteCommand( RADIO_SET_CAD, 0, 0 ) )
    {
        OperatingMode = MODE_CAD;
    }
}

void SX1280SetTxContinuousWave( void )
{
    if( SX1280WriteCommand( RADIO_SET_TXCONTINUOUSWAVE, 0, 0 ) )
    {
        OperatingMode = MODE_TX;
    }
}

void SX1280SetTxInfinitePreamble( void )
{
    if( SX1280WriteCommand( RADIO_SET_TXCONTINUOUSPREAMBLE, 0, 0 ) )
    {
        OperatingMode = MODE_TX;
    }
}

void SX1280SetRegulatorMode( RadioRegulatorMode_t mode )
{
    SX1280SetStandby( STDBY_RC ); // explicitly set to STDBY_RC since regulator mode should be set only in STDBY_RC mode
    SX1280WriteCommand( RADIO_SET_REGULATORMODE, ( uint8_t* )&mode, 1 );
}

void SX1280SetLongPreamble( uint8_t enable )
{
    SX1280WriteCommand( RADIO_SET_LONGPREAMBLE, &enable, 1 );
}

void SX1280SetDioIrqParams( uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask )
{
    uint8_t buf[8];

    buf[0] = ( uint8_t )( ( irqMask >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( irqMask & 0x00FF );
    buf[2] = ( uint8_t )( ( dio1Mask >> 8 ) & 0x00FF );
    buf[3] = ( uint8_t )( dio1Mask & 0x00FF );
    buf[4] = ( uint8_t )( ( dio2Mask >> 8 ) & 0x00FF );
    buf[5] = ( uint8_t )( dio2Mask & 0x00FF );
    buf[6] = ( uint8_t )( ( dio3Mask >> 8 ) & 0x00FF );
    buf[7] = ( uint8_t )( dio3Mask & 0x00FF );
    SX1280WriteCommand( RADIO_SET_DIOIRQPARAMS, buf, 8 );
}

uint16_t SX1280GetIrqStatus( void )
{
    uint8_t irqStatus[2] = { 0 };

    SX1280ReadCommand( RADIO_GET_IRQSTATUS, irqStatus, 2 );
    return ( (uint16_t)irqStatus[0] << 8 ) | irqStatus[1];
}

void SX1280SetRfFrequency( uint32_t frequency )
{
    uint8_t buf[3];

    uint32_t freqInPllSteps = SX1280ConvertFreqInHzToPllStep( frequency );

    buf[0] = ( uint8_t )( ( freqInPllSteps >> 16 ) & 0xFF );
    buf[1] = ( uint8_t )( ( freqInPllSteps >> 8 ) & 0xFF );
    buf[2] = ( uint8_t )( freqInPllSteps & 0xFF );
    SX1280WriteCommand( RADIO_SET_RFFREQUENCY, buf, 3 );
}

void SX1280SetPacketType( RadioPacketTypes_t packetType )
{
    // Save packet type internally to avoid questioning the radio
    if( SX1280WriteCommand( RADIO_SET_PACKETTYPE, ( uint8_t* )&packetType, 1 ) )
    {
        PacketType = packetType;
    }
}

RadioPacketTypes_t SX1280GetPacketType( void )
{
    return PacketType;
}

void SX1280SetTxParams( int8_t power, RadioRampTimes_t rampTime )
{
    uint8_t buf[2];

    if( power > 13 )
    {
        power = 13;
    }
    else if( power < -18 )
    {
        power = -18;
    }
    buf[0] = ( uint8_t )( power + 18 );
    buf[1] = ( uint8_t )rampTime;
    SX1280WriteCommand( RADIO_SET_TXPARAMS, buf, 2 );
}

void SX1280SetModulationParams( ModulationParams_t *modulationParams )
{
    uint8_t n;
    uint32_t tempVal = 0;
    uint8_t buf[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    // Check if required configuration corresponds to the stored packet type
    // If not, silently update radio packet type
    if( PacketType != modulationParams->PacketType )
    {
        SX1280SetPacketType( modulationParams->PacketType );
    }

    switch( modulationParams->PacketType )
    {
    case PACKET_TYPE_GFSK:
        n = 8;
        tempVal = ( uint32_t )( 32 * SX1280_XTAL_FREQ / modulationParams->Params.Gfsk.BitRate );
        buf[0] = ( tempVal >> 16 ) & 0xFF;
        buf[1] = ( tempVal >> 8 ) & 0xFF;
        buf[2] = tempVal & 0xFF;
        buf[3] = modulationParams->Params.Gfsk.ModulationShaping;
        buf[4] = modulationParams->Params.Gfsk.Bandwidth;
        tempVal = SX1280ConvertFreqInHzToPllStep( modulationParams->Params.Gfsk.Fdev );
        buf[5] = ( tempVal >> 16 ) & 0xFF;
        buf[6] = ( tempVal >> 8 ) & 0xFF;
        buf[7] = ( tempVal& 0xFF );
        SX1280WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, n );
        break;
    case PACKET_TYPE_LORA:
        n = 3;
        buf[0] = modulationParams->Params.LoRa.SpreadingFactor;
        buf[1] = modulationParams->Params.LoRa.Bandwidth;
        buf[2] = modulationParams->Params.LoRa.CodingRate;

        SX1280WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, n );

        break;
    default:
    case PACKET_TYPE_NONE:
        return;
    }
}

void SX1280SetPacketParams( PacketParams_t *packetParams )
{
    uint8_t n;
    uint8_t crcVal = 0;
    uint8_t buf[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    // Check if required configuration corresponds to the stored packet type
    // If not, silently update radio packet type
    if( PacketType != packetParams->PacketType )
    {
        SX1280SetPacketType( packetParams->PacketType );
    }

    switch( packetParams->PacketType )
    {
    case PACKET_TYPE_GFSK:
        if( packetParams->Params.Gfsk.CrcLength == RADIO_CRC_2_BYTES_IBM )
        {
            SX1280SetCrcSeed( CRC_IBM_SEED );
            SX1280SetCrcPolynomial( CRC_POLYNOMIAL_IBM );
            crcVal = RADIO_CRC_2_BYTES;
        }
        else if( packetParams->Params.Gfsk.CrcLength == RADIO_CRC_2_BYTES_CCIT )
        {
            SX1280SetCrcSeed( CRC_CCITT_SEED );
            SX1280SetCrcPolynomial( CRC_POLYNOMIAL_CCITT );
            crcVal = RADIO_CRC_2_BYTES_INV;
        }
        else
        {
            crcVal = packetParams->Params.Gfsk.CrcLength;
        }
        n = 9;
        buf[0] = ( packetParams->Params.Gfsk.PreambleLength >> 8 ) & 0xFF;
        buf[1] = packetParams->Params.Gfsk.PreambleLength;
        buf[2] = packetParams->Params.Gfsk.PreambleMinDetect;
        buf[3] = ( packetParams->Params.Gfsk.SyncWordLength /*<< 3*/ ); // convert from byte to bit
        buf[4] = packetParams->Params.Gfsk.AddrComp;
        buf[5] = packetParams->Params.Gfsk.HeaderType;
        buf[6] = packetParams->Params.Gfsk.PayloadLength;
        buf[7] = crcVal;
        buf[8] = packetParams->Params.Gfsk.DcFree;
        break;
    case PACKET_TYPE_LORA:
        n = 5;
        buf[0] = ( uint8_t )packetParams->Params.LoRa.PreambleLength;
        buf[1] = packetParams->Params.LoRa.HeaderType;
        buf[2] = packetParams->Params.LoRa.PayloadLength;
        buf[3] = packetParams->Params.LoRa.CrcMode;
        buf[4] = packetParams->Params.LoRa.InvertIQ;
        break;
    default:
    case PACKET_TYPE_NONE:
        return;
    }

    SX1280WriteCommand( RADIO_SET_PACKETPARAMS, buf, n );
}

void SX1280SetCadParams( RadioLoRaCadSymbols_t cadSymbolNum, uint8_t cadDetPeak, uint8_t cadDetMin, RadioCadExitModes_t cadExitMode, uint32_t cadTimeout )
{
    uint8_t buf = ( uint8_t )cadSymbolNum;

    ( void )cadDetPeak;
    ( void )cadDetMin;
    ( void )cadExitMode;
    ( void )cadTimeout;

    if( SX1280WriteCommand( RADIO_SET_CADPARAMS, &buf, 1 ) )
    {
        OperatingMode = MODE_CAD;
    }
}

void SX1280SetBufferBaseAddress( uint8_t txBaseAddress, uint8_t rxBaseAddress )
{
    uint8_t buf[2];

    buf[0] = txBaseAddress;
    buf[1] = rxBaseAddress;
    SX1280WriteCommand( RADIO_SET_BUFFERBASEADDRESS, buf, 2 );
}

RadioStatus_t SX1280GetStatus( void )
{
    uint8_t stat = 0;
    RadioStatus_t status;

    SX1280ReadCommand( RADIO_GET_STATUS, &stat, 1 );
    status.Value = stat;
    return status;
}

int8_t SX1280GetRssiInst( void )
{
    uint8_t buf[1];
    int8_t rssi = 0;

    SX1280ReadCommand( RADIO_GET_RSSIINST, buf, 1 );
    rssi = -buf[0] >> 1;
    return rssi;
}

void SX1280GetRxBufferStatus( uint8_t *payloadLength, uint8_t *rxStartBufferPointer )
{
    uint8_t status[2];

    SX1280ReadCommand( RADIO_GET_RXBUFFERSTATUS, status, 2 );

    // In case of LORA fixed header, the payloadLength is obtained by reading
    // the register REG_LR_PAYLOADLENGTH
    if( ( SX1280GetPacketType( ) == PACKET_TYPE_LORA ) && ( SX1280ReadRegister( REG_LR_PACKETPARAMS ) >> 7 == 1 ) )
    {
        *payloadLength = SX1280ReadRegister( REG_LR_PAYLOADLENGTH );
    }
    else
    {
        *payloadLength = status[0];
    }
    *rxStartBufferPointer = status[1];
}

void SX1280GetPacketStatus( PacketStatus_t *pktStatus )
{
    uint8_t status[3];

    SX1280ReadCommand( RADIO_GET_PACKETSTATUS, status, 3 );

    pktStatus->packetType = SX1280GetPacketType( );
    switch( pktStatus->packetType )
    {
        case PACKET_TYPE_GFSK:
            pktStatus->Params.Gfsk.RxStatus = status[0];
            pktStatus->Params.Gfsk.RssiSync = -status[1] >> 1;
            pktStatus->Params.Gfsk.RssiAvg = -status[2] >> 1;
            pktStatus->Params.Gfsk.FreqError = 0;
            break;

        case PACKET_TYPE_LORA:
            pktStatus->Params.LoRa.RssiPkt = -status[0] >> 1;
            // Returns SNR value [dB] rounded to the nearest integer value
            pktStatus->Params.LoRa.SnrPkt = ( ( ( int8_t )status[1] ) + 2 ) >> 2;
            pktStatus->Params.LoRa.SignalRssiPkt = -status[2] >> 1;
            pktStatus->Params.LoRa.FreqError = 0;
            break;

        default:
        case PACKET_TYPE_NONE:
            // In that specific case, we set everything in the pktStatus to zeros
            // and reset the packet type accordingly
            memset( pktStatus, 0, sizeof( PacketStatus_t ) );
            pktStatus->packetType = PACKET_TYPE_NONE;
            break;
    }
}

void SX1280ClearIrqStatus( uint16_t irq )
{
    uint8_t buf[2];

    buf[0] = ( uint8_t )( ( ( uint16_t )irq >> 8 ) & 0x00FF );
    buf[1] = ( uint8_t )( ( uint16_t )irq & 0x00FF );
    SX1280WriteCommand( RADIO_CLR_IRQSTATUS, buf, 2 );
}

static uint32_t SX1280ConvertFreqInHzToPllStep( uint32_t freqInHz )
{
    uint32_t stepsInt;
    uint32_t stepsFrac;

    // pllSteps = freqInHz / (SX1280_XTAL_FREQ / 2^18)
    // Get integer and fractional parts of the frequency computed with a PLL step scaled value
    stepsInt = freqInHz / SX1280_PLL_STEP_SCALED;
    stepsFrac = freqInHz - ( stepsInt * SX1280_PLL_STEP_SCALED );
    
    // Apply the scaling factor to retrieve a frequency in Hz (+ ceiling)
    return ( stepsInt << SX1280_PLL_STEP_SHIFT_AMOUNT ) +
           ( ( ( stepsFrac << SX1280_PLL_STEP_SHIFT_AMOUNT ) + ( SX1280_PLL_STEP_SCALED >> 1 ) ) /
             SX1280_PLL_STEP_SCALED );
}

#endif /* RADIO_ENABLE */
