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
    /* Kept identical to b6c7290 (GFSK validated). FLRC writes its 4-byte
     * sync word inline in RadioSetRxConfig/TxConfig MODEM_FLRC branches
     * instead of routing through this function — adding a packet-type
     * switch here empirically broke GFSK demodulation (2026-05-21 test). */
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
    /* SX1280 whitening seed is 8-bit (NOT 9-bit like SX126x). Official Semtech
     * SX1280 driver (sx1280-driver-c/sx1280.c SX1280SetWhiteningSeed) writes a
     * single byte at REG_LR_WHITSEEDBASEADDR (0x09C5). Previous implementation
     * was inherited from the SX126x driver and wrote 2 bytes (0x09C5 + 0x09C6),
     * but 0x09C6 == REG_LR_CRCPOLYBASEADDR MSB -> every SetWhiteningSeed() call
     * silently clobbered the CRC polynomial MSB to (uint8_t)seed, breaking the
     * chip's CRC engine.  Keep the uint16_t prototype for ABI; only LSB used. */
    switch( SX1280GetPacketType( ) )
    {
        case PACKET_TYPE_GFSK:
            SX1280WriteRegister( REG_LR_WHITSEEDBASEADDR, ( uint8_t )seed );
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

/* SX1280 GFSK SetModulationParams / SetPacketParams use a 3-byte / 7-byte
 * lookup-encoded format, NOT the SX1262-style raw register values used by the
 * baseline driver. The lookup tables below mirror the official Semtech SX1280
 * driver (SWSD001). LoRa code paths are unchanged. */
typedef struct { uint32_t bitrate; uint32_t bandwidth; uint8_t param; } SX1280GfskBrBw_t;
static const SX1280GfskBrBw_t SX1280GfskBrBw[] = {
    { 125000,  300000,  0xEF }, { 250000,  300000,  0xC7 }, { 250000,  600000,  0xCE },
    { 400000,  600000,  0xAA }, { 500000,  600000,  0x86 }, { 400000,  1200000, 0xB1 },
    { 500000,  1200000, 0x8D }, { 800000,  1200000, 0x69 }, { 1000000, 1200000, 0x45 },
    { 800000,  2400000, 0x70 }, { 1000000, 2400000, 0x4C }, { 1600000, 2400000, 0x28 },
    { 2000000, 2400000, 0x04 },
};
typedef struct { uint8_t numerator; uint8_t param; } SX1280GfskModInd_t;
static const SX1280GfskModInd_t SX1280GfskModInd[] = {
    { 7,0x00 },{ 10,0x01 },{ 15,0x02 },{ 20,0x03 },{ 25,0x04 },{ 30,0x05 },{ 35,0x06 },{ 40,0x07 },
    { 45,0x08 },{ 50,0x09 },{ 55,0x0A },{ 60,0x0B },{ 65,0x0C },{ 70,0x0D },{ 75,0x0E },{ 80,0x0F },
};
static uint8_t SX1280GetGfskBrBwParam( uint32_t bitrate, uint32_t bandwidth ) {
    for( uint8_t i = 0; i < sizeof(SX1280GfskBrBw)/sizeof(SX1280GfskBrBw[0]); i++ ) {
        if( bitrate <= SX1280GfskBrBw[i].bitrate && bandwidth <= SX1280GfskBrBw[i].bandwidth ) return SX1280GfskBrBw[i].param;
    }
    return SX1280GfskBrBw[sizeof(SX1280GfskBrBw)/sizeof(SX1280GfskBrBw[0]) - 1].param;
}
static uint8_t SX1280GetGfskModIndParam( uint32_t bitrate, uint32_t fdev ) {
    if( bitrate == 0 ) return SX1280GfskModInd[0].param;
    for( int8_t i = (sizeof(SX1280GfskModInd)/sizeof(SX1280GfskModInd[0])) - 1; i >= 0; i-- ) {
        if( ( bitrate * SX1280GfskModInd[i].numerator ) <= ( 40U * fdev ) ) return SX1280GfskModInd[i].param;
    }
    return SX1280GfskModInd[0].param;
}
static uint8_t SX1280GetGfskPulseShapeParam( RadioModShapings_t shaping ) {
    switch( shaping ) {
    case MOD_SHAPING_G_BT_05: return 0x20;
    case MOD_SHAPING_G_BT_1:  return 0x10;
    case MOD_SHAPING_OFF: default: return 0x00;
    }
}
static uint8_t SX1280GetGfskPreambleLenParam( uint16_t preambleLenBits ) {
    if( preambleLenBits <= 4 ) return 0x00;
    if( preambleLenBits >= 32 ) return 0x70;
    return ( uint8_t )( ( ( preambleLenBits + 3 ) / 4 - 1 ) << 4 );
}
static uint8_t SX1280GetGfskSyncWordLenParam( uint8_t syncWordLenInBits ) {
    /* SX1280 SetPacketParams byte[1] encodes sync word length in bytes:
     *   0x00 = 1B, 0x02 = 2B, 0x04 = 3B, 0x06 = 4B, 0x08 = 5B.
     * Caller passes bits (n_bytes << 3). Previous version returned 0x08 for
     * any input >=5 — that broke LWB which uses 3-byte sync word (passes
     * 24 bits, was being encoded as 5 bytes → TX/RX length mismatch → sync
     * never matched). */
    uint8_t n_bytes = syncWordLenInBits / 8;
    if (n_bytes < 1) n_bytes = 1;
    if (n_bytes > 5) n_bytes = 5;
    return (uint8_t)((n_bytes - 1) << 1);
}
static uint8_t SX1280GetGfskCrcParam( RadioCrcTypes_t crc ) {
    switch( crc ) {
    case RADIO_CRC_1_BYTES: return 0x10;
    case RADIO_CRC_2_BYTES: case RADIO_CRC_2_BYTES_IBM: case RADIO_CRC_2_BYTES_CCIT: return 0x20;
    case RADIO_CRC_3_BYTES: return 0x30;    /* FLRC-only; GFSK datasheet 14.5.2 also accepts 0x30 */
    case RADIO_CRC_OFF: default: return 0x00;
    }
}
static uint8_t SX1280GetGfskWhiteningParam( RadioDcFree_t dcFree ) {
    return ( dcFree == RADIO_DC_FREEWHITENING ) ? 0x00 : 0x08;
}

void SX1280SetModulationParams( ModulationParams_t *modulationParams )
{
    uint8_t n;
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
        /* SX1280 lookup-encoded GFSK modulation params (3 bytes). */
        n = 3;
        buf[0] = SX1280GetGfskBrBwParam( modulationParams->Params.Gfsk.BitRate,
                                         modulationParams->Params.Gfsk.Bandwidth );
        buf[1] = SX1280GetGfskModIndParam( modulationParams->Params.Gfsk.BitRate,
                                           modulationParams->Params.Gfsk.Fdev );
        buf[2] = SX1280GetGfskPulseShapeParam( modulationParams->Params.Gfsk.ModulationShaping );
        SX1280WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, n );
        break;
    case PACKET_TYPE_LORA:
        n = 3;
        buf[0] = modulationParams->Params.LoRa.SpreadingFactor;
        buf[1] = modulationParams->Params.LoRa.Bandwidth;
        buf[2] = modulationParams->Params.LoRa.CodingRate;

        SX1280WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, n );

        break;
    case PACKET_TYPE_FLRC:
        /* FLRC: 3 bytes pre-encoded chip values (datasheet 14.6.5). */
        n = 3;
        buf[0] = ( uint8_t )modulationParams->Params.Flrc.BitrateBandwidth;
        buf[1] = ( uint8_t )modulationParams->Params.Flrc.CodingRate;
        buf[2] = SX1280GetGfskPulseShapeParam( modulationParams->Params.Flrc.ModulationShaping );
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
        /* SX1280 lookup-encoded GFSK packet params (7 bytes). CRC seed/polynomial
         * are still set explicitly here for IBM/CCITT modes (chip uses register-
         * driven CRC seed when RADIO_CRC_2_BYTES_* is selected). */
        if( packetParams->Params.Gfsk.CrcLength == RADIO_CRC_2_BYTES_IBM )
        {
            SX1280SetCrcSeed( CRC_IBM_SEED );
            SX1280SetCrcPolynomial( CRC_POLYNOMIAL_IBM );
        }
        else if( packetParams->Params.Gfsk.CrcLength == RADIO_CRC_2_BYTES_CCIT )
        {
            SX1280SetCrcSeed( CRC_CCITT_SEED );
            SX1280SetCrcPolynomial( CRC_POLYNOMIAL_CCITT );
        }
        n = 7;
        buf[0] = SX1280GetGfskPreambleLenParam( packetParams->Params.Gfsk.PreambleLength );
        buf[1] = SX1280GetGfskSyncWordLenParam( packetParams->Params.Gfsk.SyncWordLength );
        /* SX1280 GFSK PacketParam3: MatchSyncWord select bits (datasheet
         * table 14.45), NOT a generic address compare. The Semtech driver
         * mislabels this field as "AddrComp" and sets it to FILT_OFF=0x00
         * which actually DISABLES sync word matching entirely — RX would
         * never declare SyncValid/RxDone. Force RADIO_RX_MATCH_SYNCWORD_1
         * (0x10) so sync word 1 is matched. Verified via two-node
         * gfsk_test on user's DLP-RFS1280 + SX1280 fw 0xA9B7. */
        buf[2] = 0x10;
        buf[3] = packetParams->Params.Gfsk.HeaderType;
        buf[4] = packetParams->Params.Gfsk.PayloadLength;
        buf[5] = SX1280GetGfskCrcParam( packetParams->Params.Gfsk.CrcLength );
        buf[6] = SX1280GetGfskWhiteningParam( packetParams->Params.Gfsk.DcFree );
        break;
    case PACKET_TYPE_LORA:
        n = 5;
        buf[0] = ( uint8_t )packetParams->Params.LoRa.PreambleLength;
        buf[1] = packetParams->Params.LoRa.HeaderType;
        buf[2] = packetParams->Params.LoRa.PayloadLength;
        buf[3] = packetParams->Params.LoRa.CrcMode;
        buf[4] = packetParams->Params.LoRa.InvertIQ;
        break;
    case PACKET_TYPE_FLRC:
        /* FLRC: 7 bytes (datasheet 14.6.6 PacketParam1..7). */
        n = 7;
        buf[0] = SX1280GetGfskPreambleLenParam( packetParams->Params.Flrc.PreambleLength );
        buf[1] = ( uint8_t )packetParams->Params.Flrc.SyncWordLength;
        buf[2] = packetParams->Params.Flrc.SyncWordMatch;
        buf[3] = packetParams->Params.Flrc.HeaderType;
        buf[4] = packetParams->Params.Flrc.PayloadLength;
        buf[5] = SX1280GetGfskCrcParam( packetParams->Params.Flrc.CrcLength );
        buf[6] = SX1280GetGfskWhiteningParam( packetParams->Params.Flrc.DcFree );
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

        case PACKET_TYPE_FLRC:
            /*
             * FLRC GetPacketStatus has the same RSSI byte position as GFSK:
             * packetStatus[15:8] is rssiSync and its value is -2 * dBm.
             * Keep using the GFSK storage for compatibility with Radio.RxDone;
             * FLRC has no packet SNR or averaged-RSSI field.
             */
            pktStatus->Params.Gfsk.RxStatus = status[2];
            pktStatus->Params.Gfsk.RssiSync = -( int8_t )( status[1] / 2U );
            pktStatus->Params.Gfsk.RssiAvg = pktStatus->Params.Gfsk.RssiSync;
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
