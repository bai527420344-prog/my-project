/*!
 * \file      radio.c
 *
 * \brief     Radio driver API definition
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
 * reference implementation is available at https://github.com/Lora-net/LoRaMac-node/blob/master/src/radio/sx1280/radio.c
 */


#include "flora_lib.h"

#if RADIO_ENABLE

#define RADIO_MAX_PAYLOAD_SIZE      255       // max. number of payload bytes (must not exceed 255!)


/*!
 * \brief Initializes the radio
 *
 * \param [IN] events Structure containing the driver callback functions
 */
void RadioInit( RadioEvents_t *events );

/*!
 * Return current radio status
 *
 * \param status Radio status.[RF_IDLE, RF_RX_RUNNING, RF_TX_RUNNING]
 */
RadioState_t RadioGetStatus( void );

/*!
 * \brief Configures the radio with the given modem
 *
 * \param [IN] modem Modem to be used [0: FSK, 1: LoRa]
 */
void RadioSetModem( RadioModems_t modem );

/*!
 * \brief Sets the channel frequency
 *
 * \param [IN] freq         Channel RF frequency
 */
void RadioSetChannel( uint32_t freq );

/*!
 * \brief Checks if the channel is free for the given time
 *
 * \param [IN] modem      Radio modem to be used [0: FSK, 1: LoRa]
 * \param [IN] freq       Channel RF frequency
 * \param [IN] rssiThresh RSSI threshold
 * \param [IN] maxCarrierSenseTime Max time while the RSSI is measured
 *
 * \retval isFree         [true: Channel is free, false: Channel is not free]
 */
bool RadioIsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime );

/*!
 * \brief Generates a 32 bits random value based on the RSSI readings
 *
 * \remark This function sets the radio in LoRa modem mode and disables
 *         all interrupts.
 *         After calling this function either Radio.SetRxConfig or
 *         Radio.SetTxConfig functions must be called.
 *
 * \retval randomValue    32 bits random value
 */
uint32_t RadioRandom( void );

/*!
 * \brief Sets the reception parameters
 *
 * \param [IN] modem        Radio modem to be used [0: FSK, 1: LoRa]
 * \param [IN] bandwidth    Sets the bandwidth
 *                          FSK : >= 2600 and <= 250000 Hz
 *                          LoRa: [0: 125 kHz, 1: 250 kHz,
 *                                 2: 500 kHz, 3: Reserved]
 * \param [IN] datarate     Sets the Datarate
 *                          FSK : 600..300000 bits/s
 *                          LoRa: [5: 32, 6: 64, 7: 128, 8: 256, 9: 512,
 *                                10: 1024, 11: 2048, 12: 4096  chips]
 * \param [IN] coderate     Sets the coding rate (LoRa only)
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
 * \param [IN] bandwidthAfc Sets the AFC Bandwidth (FSK only)
 *                          FSK : >= 2600 and <= 250000 Hz
 *                          LoRa: N/A ( set to 0 )
 * \param [IN] preambleLen  Sets the Preamble length
 *                          FSK : Number of bytes
 *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
 * \param [IN] symbTimeout  Sets the RxSingle timeout value
 *                          FSK : timeout in number of bytes
 *                          LoRa: timeout in symbols
 * \param [IN] fixLen       Fixed length packets [0: variable, 1: fixed]
 * \param [IN] payloadLen   Sets payload length when fixed length is used, or the max.
 *                          allowed payload length in explicit mode
 * \param [IN] crcOn        Enables/Disables the CRC [0: OFF, 1: ON]
 * \param [IN] FreqHopOn    Enables disables the intra-packet frequency hopping
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: [0: OFF, 1: ON]
 * \param [IN] HopPeriod    Number of symbols between each hop
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: Number of symbols
 * \param [IN] iqInverted   Inverts IQ signals (LoRa only)
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: [0: not inverted, 1: inverted]
 */
void RadioSetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                          uint32_t datarate, uint8_t coderate,
                          uint32_t bandwidthAfc, uint16_t preambleLen,
                          uint16_t symbTimeout, bool fixLen,
                          uint8_t payloadLen,
                          bool crcOn, bool FreqHopOn, uint8_t HopPeriod,
                          bool iqInverted );

/*!
 * \brief Sets the transmission parameters
 *
 * \param [IN] modem        Radio modem to be used [0: FSK, 1: LoRa]
 * \param [IN] power        Sets the output power [dBm]
 * \param [IN] fdev         Sets the frequency deviation (FSK only)
 *                          FSK : [Hz]
 *                          LoRa: 0
 * \param [IN] bandwidth    Sets the bandwidth (LoRa only)
 *                          FSK : 0
 *                          LoRa: [0: 125 kHz, 1: 250 kHz,
 *                                 2: 500 kHz, 3: Reserved]
 * \param [IN] datarate     Sets the Datarate
 *                          FSK : 600..300000 bits/s
 *                          LoRa: [5: 32, 6: 64, 7: 128, 8: 256, 9: 512,
 *                                10: 1024, 11: 2048, 12: 4096  chips]
 * \param [IN] coderate     Sets the coding rate (LoRa only)
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
 * \param [IN] preambleLen  Sets the preamble length
 *                          FSK : Number of bytes
 *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
 * \param [IN] fixLen       Fixed length packets [0: variable, 1: fixed]
 * \param [IN] crcOn        Enables disables the CRC [0: OFF, 1: ON]
 * \param [IN] FreqHopOn    Enables disables the intra-packet frequency hopping
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: [0: OFF, 1: ON]
 * \param [IN] HopPeriod    Number of symbols between each hop
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: Number of symbols
 * \param [IN] iqInverted   Inverts IQ signals (LoRa only)
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: [0: not inverted, 1: inverted]
 * \param [IN] timeout      Transmission timeout [ms]
 */
void RadioSetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                          uint32_t bandwidth, uint32_t datarate,
                          uint8_t coderate, uint16_t preambleLen,
                          bool fixLen, bool crcOn, bool FreqHopOn,
                          uint8_t HopPeriod, bool iqInverted, uint32_t timeout );

/*!
 * \brief Checks if the given RF frequency is supported by the hardware
 *
 * \param [IN] frequency RF frequency to be checked
 * \retval isSupported [true: supported, false: unsupported]
 */
bool RadioCheckRfFrequency( uint32_t frequency );

/*!
 * \brief Computes the packet time on air in ms for the given payload
 *
 * \param [IN] modem        Radio modem to be used [0: FSK, 1: LoRa]
 * \param [IN] bandwidth    Sets the bandwidth
 *                          FSK : >= 2600 and <= 250000 Hz
 *                          LoRa: [0: 125 kHz, 1: 250 kHz,
 *                                 2: 500 kHz, 3: Reserved]
 * \param [IN] datarate     Sets the Datarate
 *                          FSK : 600..300000 bits/s
 *                          LoRa: [5: 32, 6: 64, 7: 128, 8: 256, 9: 512,
 *                                10: 1024, 11: 2048, 12: 4096  chips]
 * \param [IN] coderate     Sets the coding rate (LoRa only)
 *                          FSK : N/A ( set to 0 )
 *                          LoRa: [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
 * \param [IN] preambleLen  Sets the Preamble length
 *                          FSK : Number of bytes
 *                          LoRa: Length in symbols (the hardware adds 4 more symbols)
 * \param [IN] fixLen       Fixed length packets [0: variable, 1: fixed]
 * \param [IN] payloadLen   Sets payload length when fixed length is used
 * \param [IN] crcOn        Enables/Disables the CRC [0: OFF, 1: ON]
 *
 * \retval airTime          Computed airTime (us) for the given packet payload length
 */
uint32_t RadioTimeOnAir( RadioModems_t modem, uint32_t bandwidth,
                              uint32_t datarate, uint8_t coderate,
                              uint16_t preambleLen, bool fixLen, uint8_t payloadLen,
                              bool crcOn );

/*!
 * \brief Sends the buffer of size. Prepares the packet to be sent and sets
 *        the radio in transmission mode
 *
 * \param [IN]: buffer     Buffer pointer
 * \param [IN]: size       Buffer size
 */
void RadioSendPayload( uint8_t *buffer, uint8_t size );

/*!
 * \brief Sends the buffer of size. Prepares the packet to be sent and sets
 *        the radio in transmission mode with configuring
 *        the radio interrupts by the mask
 *
 * \param [IN]: mask       Mask to enable/disable radio interrupts
 * \param [IN]: buffer     Buffer pointer
 * \param [IN]: size       Buffer size
 */
void RadioSendPayloadMask( uint16_t mask, uint8_t *buffer, uint8_t size );

/*!
 * \brief Sets the radio in sleep mode
 */
void RadioSleep( void );

/*!
 * \brief Sets the radio in cold sleep mode
 */
void RadioColdSleep( void );

/*!
 * \brief Sets the radio in standby mode
 */
void RadioStandby( void );

/*!
 * \brief Sets the radio in reception mode for the given time
 */
void RadioRx( uint32_t timeout_ms, bool continuous, bool scheduled );

/*!
 * \brief Sets the radio in reception mode for the given time with configuring
 *        the radio interrupts by the mask
 */
void RadioRxMask( uint16_t mask, uint32_t timeout_ms, bool continuous, bool scheduled );

/*!
 * \brief Start a Channel Activity Detection
 */
void RadioStartCad( void );

/*!
 * \brief Sets the radio into TX mode
 */
void RadioTx( uint32_t timeout_ms, bool scheduled );

/*!
 * \brief Sets the radio into TX mode with configuring
 *        the radio interrupts by the mask
 */
void RadioTxMask( uint16_t mask, uint32_t timeout_ms, bool scheduled );

/*!
 * \brief Sets the radio in continuous wave transmission mode
 *
 * \param [IN]: freq       Channel RF frequency
 * \param [IN]: power      Sets the output power [dBm]
 */
void RadioSetTxContinuousWave( uint32_t freq, int8_t power );

/*!
 * \brief Reads the current RSSI value
 *
 * \retval rssiValue Current RSSI value in [dBm]
 */
int16_t RadioRssi( RadioModems_t modem );

/*!
 * \brief Writes the radio register at the specified address
 *
 * \param [IN]: addr Register address
 * \param [IN]: data New register value
 */
void RadioWrite( uint16_t addr, uint8_t data );

/*!
 * \brief Reads the radio register at the specified address
 *
 * \param [IN]: addr Register address
 * \retval data Register value
 */
uint8_t RadioRead( uint16_t addr );

/*!
 * \brief Writes multiple radio registers starting at address
 *
 * \param [IN] addr   First Radio register address
 * \param [IN] buffer Buffer containing the new register's values
 * \param [IN] size   Number of registers to be written
 */
void RadioWriteBuffer( uint16_t addr, uint8_t *buffer, uint8_t size );

/*!
 * \brief Reads multiple radio registers starting at address
 *
 * \param [IN] addr First Radio register address
 * \param [OUT] buffer Buffer where to copy the registers data
 * \param [IN] size Number of registers to be read
 */
void RadioReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size );

/*!
 * \brief Sets the maximum payload length.
 *
 * \param [IN] max        Maximum payload length in bytes
 *
 * \note Packet type must be set with RadioSetModem() before calling this function.
 */
void RadioSetMaxPayloadLength( uint8_t max );

/*!
 * \brief Sets the whitening mode.
 *
 * \param [IN] whitening  Whitening mode [0: no whitening, 1: whitening enabled]
 */
void RadioSetGfskWhitening( uint8_t whitening );

/*!
 * \brief Sets sync word length.
 *
 * \param [IN] syncWordLength      Sync word length [number of bytes]
 */
void RadioSetGfskSyncWordLength( uint8_t syncWordLength );

/*!
 * \brief Sets sync word.
 *
 * \param [IN] syncWord   Sync word (fixed length of 8 bytes)
 */
void RadioSetGfskSyncWord( uint8_t syncWord[] );

/*!
 * \brief Sets the network to public or private. Updates the sync byte.
 *
 * \remark Applies to LoRa modem only
 *
 * \param [IN] enable if true, it enables a public network
 */
void RadioSetPublicNetwork( bool enable );

/*!
 * \brief Gets the time required for the board plus radio to get out of sleep.[ms]
 *
 * \retval time Radio plus board wakeup time in ms.
 */
uint32_t RadioGetWakeupTime( void );

/*!
 * \brief Process radio irq
 */
void RadioIrqProcess( void );

/*!
 * \brief Sets the radio in reception mode with Max LNA gain for the given time
 */
void RadioRxBoosted( uint32_t timeout_ms, bool continuous, bool scheduled );

/*!
 * \brief Sets the radio in reception mode with Max LNA gain for the given time
 */
void RadioRxBoostedMask( uint16_t mask, uint32_t timeout_ms, bool continuous, bool scheduled );

/*!
 * \brief Sets the Rx duty cycle management parameters
 */
void RadioSetRxDutyCycle( uint32_t rxTime, uint32_t sleepTime, bool scheduled );

/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio =
{
    RadioInit,
    RadioGetStatus,
    RadioSetModem,
    RadioSetChannel,
    RadioIsChannelFree,
    RadioRandom,
    RadioSetRxConfig,
    RadioSetTxConfig,
    RadioCheckRfFrequency,
    RadioTimeOnAir,
    RadioSendPayload,
    RadioSendPayloadMask,
    RadioSleep,
    RadioColdSleep,
    RadioStandby,
    RadioRx,
    RadioRxMask,
    RadioStartCad,
    RadioSetTxContinuousWave,
    RadioRssi,
    RadioWrite,
    RadioRead,
    RadioWriteBuffer,
    RadioReadBuffer,
    RadioSetMaxPayloadLength,
    RadioSetPublicNetwork,
    RadioGetWakeupTime,
    RadioIrqProcess,
    // Available on SX1280 only
    RadioRxBoosted,
    RadioRxBoostedMask,
    RadioSetRxDutyCycle,
    RadioTx,
    RadioTxMask
};

/*
 * Local types definition
 */


 /*!
 * FSK bandwidth definition
 */
typedef struct
{
    uint32_t bandwidth;
    uint8_t  RegValue;
}FskBandwidth_t;

/*!
 * Precomputed FSK bandwidth registers values
 */
const FskBandwidth_t FskBandwidths[] =
{
    { 4800  , 0x1F },
    { 5800  , 0x17 },
    { 7300  , 0x0F },
    { 9700  , 0x1E },
    { 11700 , 0x16 },
    { 14600 , 0x0E },
    { 19500 , 0x1D },
    { 23400 , 0x15 },
    { 29300 , 0x0D },
    { 39000 , 0x1C },
    { 46900 , 0x14 },
    { 58600 , 0x0C },
    { 78200 , 0x1B },
    { 93800 , 0x13 },
    { 117300, 0x0B },
    { 156200, 0x1A },
    { 187200, 0x12 },
    { 234300, 0x0A },
    { 312000, 0x19 },
    { 373600, 0x11 },
    { 467000, 0x09 },
    { 500000, 0x00 }, // Invalid Bandwidth
};

const RadioLoRaBandwidths_t Bandwidths[] = { LORA_BW_0200, LORA_BW_0400, LORA_BW_0800, LORA_BW_1600 };

uint8_t MaxPayloadLength = 0xFF;

uint8_t GfskWhitening = RADIO_DC_FREEWHITENING;

uint8_t GfskSyncWordLength = 3;
// uint8_t GfskSyncWordLength = 4; // for comparison with glossy CC430

uint8_t GfskSyncWord[8] = { 0xC1, 0x94, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00 };

uint8_t RadioRxPayload[RADIO_MAX_PAYLOAD_SIZE + 1] = { 0 };

bool IrqFired = false;

/*
 * SX1280 DIO IRQ callback functions prototype
 */

/*!
 * \brief DIO 0 IRQ callback
 */
void RadioOnDioIrq( void );

void (*RadioOnDioIrqCallback)( void ) = &RadioOnDioIrq;

/*!
 * \brief Tx timeout timer callback
 */
void RadioOnTxTimeoutIrq( void );

/*!
 * \brief Rx timeout timer callback
 */
void RadioOnRxTimeoutIrq( void );

/*
 * Private global variables
 */


/*!
 * Holds the current network type for the radio
 */
typedef struct
{
    bool Previous;
    bool Current;
}RadioPublicNetwork_t;

static RadioPublicNetwork_t RadioPublicNetwork = { false };

/*!
 * Radio callbacks variable
 */
static RadioEvents_t* RadioEvents;

/*
 * Public global variables
 */

/*!
 * Radio hardware and global parameters
 */
SX1280_t SX1280;


/*!
 * Returns the known FSK bandwidth registers value
 *
 * \param [IN] bandwidth Bandwidth value in Hz
 * \retval regValue Bandwidth register value.
 */
static uint8_t RadioGetFskBandwidthRegValue( uint32_t bandwidth )
{
    uint8_t i;

    if( bandwidth == 0 )
    {
        return( 0x1F );
    }

    for( i = 0; i < ( sizeof( FskBandwidths ) / sizeof( FskBandwidth_t ) ) - 1; i++ )
    {
        if( ( bandwidth >= FskBandwidths[i].bandwidth ) && ( bandwidth < FskBandwidths[i + 1].bandwidth ) )
        {
            return FskBandwidths[i+1].RegValue;
        }
    }

    return FskBandwidths[(sizeof( FskBandwidths ) / sizeof( FskBandwidth_t )) - 2].RegValue;
}

void RadioInit( RadioEvents_t *events )
{
    RadioEvents = events;

    RadioOnDioIrqCallback = *RadioOnDioIrq;
    SX1280Init();

    SX1280SetStandby( STDBY_RC );
    SX1280SetRegulatorMode( USE_DCDC );

    SX1280SetBufferBaseAddress( 0x00, 0x00 );
    SX1280SetTxParams( 0, RADIO_RAMP_20_US );
    SX1280SetDioIrqParams( IRQ_RADIO_ALL, IRQ_RADIO_ALL, IRQ_RADIO_NONE, IRQ_RADIO_NONE );

    IrqFired = false;
}

RadioState_t RadioGetStatus( void )
{
    switch( SX1280GetOperatingMode( ) )
    {
        case MODE_TX:
            return RF_TX_RUNNING;
        case MODE_RX:
        case MODE_RX_DC:
        case MODE_RX_CONTINUOUS:
            return RF_RX_RUNNING;
        case MODE_CAD:
            return RF_CAD;
        default:
            return RF_IDLE;
    }
}

void RadioSetModem( RadioModems_t modem )
{
    switch( modem )
    {
    default:
    case MODEM_FSK:
        SX1280SetPacketType( PACKET_TYPE_GFSK );
        // When switching to GFSK mode the LoRa SyncWord register value is reset
        // Thus, we also reset the RadioPublicNetwork variable
        RadioPublicNetwork.Current = false;
        break;
    case MODEM_LORA:
        SX1280SetPacketType( PACKET_TYPE_LORA );
        // Public/Private network register is reset when switching modems
        if( RadioPublicNetwork.Current != RadioPublicNetwork.Previous )
        {
            RadioPublicNetwork.Current = RadioPublicNetwork.Previous;
            RadioSetPublicNetwork( RadioPublicNetwork.Current );
        }
        break;
    }

}

void RadioSetChannel( uint32_t freq )
{
    SX1280SetRfFrequency( freq );
}

bool RadioIsChannelFree( RadioModems_t modem, uint32_t freq, int16_t rssiThresh, uint32_t maxCarrierSenseTime )
{
    bool status = true;
    volatile int16_t rssi = 0;
    uint64_t carrierSenseTime = 0;

    RadioSetModem( modem );
    RadioSetChannel( freq );

    RadioRx( 0, false, false );

    rtc_delay(30);

    carrierSenseTime = rtc_get_timestamp(false);

    // Perform carrier sense for maxCarrierSenseTime
    while( (rtc_get_timestamp(false) - carrierSenseTime) < maxCarrierSenseTime )
    {
        rssi = RadioRssi( modem );
        if( rssi > rssiThresh )
        {
            status = false;
            break;
        }
    }
    RadioSleep( );
    return status;
}

uint32_t RadioRandom( void )
{
    uint32_t rnd = 0;

    /*
     * Radio setup for random number generation
     */
    // Set LoRa modem ON
    RadioSetModem( MODEM_LORA );

    // Disable LoRa modem interrupts
    SX1280SetDioIrqParams( IRQ_RADIO_NONE, IRQ_RADIO_NONE, IRQ_RADIO_NONE, IRQ_RADIO_NONE );

    rnd = SX1280GetRandom( );

    return rnd;
}

void RadioSetRxConfig( RadioModems_t modem, uint32_t bandwidth,
                         uint32_t datarate, uint8_t coderate,
                         uint32_t bandwidthAfc, uint16_t preambleLen,
                         uint16_t symbTimeout, bool fixLen,
                         uint8_t payloadLen,
                         bool crcOn, bool freqHopOn, uint8_t hopPeriod,
                         bool iqInverted )
{
    MaxPayloadLength = payloadLen;

    switch( modem )
    {
        case MODEM_FSK:
            SX1280.ModulationParams.PacketType = PACKET_TYPE_GFSK;

            SX1280.ModulationParams.Params.Gfsk.BitRate = datarate;
            SX1280.ModulationParams.Params.Gfsk.ModulationShaping = MOD_SHAPING_G_BT_1;
            SX1280.ModulationParams.Params.Gfsk.Bandwidth = RadioGetFskBandwidthRegValue( bandwidth );

            SX1280.PacketParams.PacketType = PACKET_TYPE_GFSK;
            SX1280.PacketParams.Params.Gfsk.PreambleLength = ( preambleLen << 3 ); // convert byte into bit
            SX1280.PacketParams.Params.Gfsk.PreambleMinDetect = RADIO_PREAMBLE_DETECTOR_08_BITS;
            SX1280.PacketParams.Params.Gfsk.SyncWordLength = GfskSyncWordLength << 3; // convert byte into bit
            SX1280.PacketParams.Params.Gfsk.AddrComp = RADIO_ADDRESSCOMP_FILT_OFF;
            SX1280.PacketParams.Params.Gfsk.HeaderType = ( fixLen == true ) ? RADIO_PACKET_FIXED_LENGTH : RADIO_PACKET_VARIABLE_LENGTH;
            SX1280.PacketParams.Params.Gfsk.PayloadLength = MaxPayloadLength;
            if( crcOn == true )
            {
                SX1280.PacketParams.Params.Gfsk.CrcLength = RADIO_CRC_2_BYTES_CCIT;
            }
            else
            {
                SX1280.PacketParams.Params.Gfsk.CrcLength = RADIO_CRC_OFF;
            }
            SX1280.PacketParams.Params.Gfsk.DcFree = GfskWhitening;

            RadioStandby( );
            RadioSetModem( MODEM_FSK );
            SX1280SetModulationParams( &SX1280.ModulationParams );
            SX1280SetPacketParams( &SX1280.PacketParams );
            SX1280SetSyncWord( GfskSyncWord );
            SX1280SetWhiteningSeed( 0x01FF );

            break;

        case MODEM_LORA:
            SX1280.ModulationParams.PacketType = PACKET_TYPE_LORA;
            SX1280.ModulationParams.Params.LoRa.SpreadingFactor = ( RadioLoRaSpreadingFactors_t )(datarate << 4);
            SX1280.ModulationParams.Params.LoRa.Bandwidth = Bandwidths[bandwidth];
            SX1280.ModulationParams.Params.LoRa.CodingRate = ( RadioLoRaCodingRates_t )coderate;
            SX1280.ModulationParams.Params.LoRa.LowDatarateOptimize = 0x00;

            SX1280.PacketParams.PacketType = PACKET_TYPE_LORA;

            // NOTE: do not increase preamble length to 12 for SF5 and SF6 if it is below 12

            SX1280.PacketParams.Params.LoRa.PreambleLength = preambleLen;

            /* Bool cast to enum yielded 0x01 for fixed-length, but SX1280
             * expects 0x80 (datasheet 14.4.3). Use proper enum names. */
            SX1280.PacketParams.Params.LoRa.HeaderType = ( fixLen == true ) ? LORA_PACKET_FIXED_LENGTH : LORA_PACKET_VARIABLE_LENGTH;

            SX1280.PacketParams.Params.LoRa.PayloadLength = MaxPayloadLength;
            SX1280.PacketParams.Params.LoRa.CrcMode = ( RadioLoRaCrcModes_t )crcOn;
            SX1280.PacketParams.Params.LoRa.InvertIQ = ( RadioLoRaIQModes_t )iqInverted;

            RadioStandby( );
            RadioSetModem( MODEM_LORA );
            SX1280SetModulationParams( &SX1280.ModulationParams );
            SX1280SetPacketParams( &SX1280.PacketParams );

            break;
    }
}

void RadioSetTxConfig( RadioModems_t modem, int8_t power, uint32_t fdev,
                        uint32_t bandwidth, uint32_t datarate,
                        uint8_t coderate, uint16_t preambleLen,
                        bool fixLen, bool crcOn, bool freqHopOn,
                        uint8_t hopPeriod, bool iqInverted, uint32_t timeout )
{
    switch( modem )
    {
        case MODEM_FSK:
            SX1280.ModulationParams.PacketType = PACKET_TYPE_GFSK;
            SX1280.ModulationParams.Params.Gfsk.BitRate = datarate;

            SX1280.ModulationParams.Params.Gfsk.ModulationShaping = MOD_SHAPING_G_BT_1;
            SX1280.ModulationParams.Params.Gfsk.Bandwidth = RadioGetFskBandwidthRegValue( bandwidth );
            SX1280.ModulationParams.Params.Gfsk.Fdev = fdev;

            SX1280.PacketParams.PacketType = PACKET_TYPE_GFSK;
            SX1280.PacketParams.Params.Gfsk.PreambleLength = ( preambleLen << 3 ); // convert byte into bit
            SX1280.PacketParams.Params.Gfsk.PreambleMinDetect = RADIO_PREAMBLE_DETECTOR_08_BITS;
            SX1280.PacketParams.Params.Gfsk.SyncWordLength = GfskSyncWordLength << 3 ; // convert byte into bit
            SX1280.PacketParams.Params.Gfsk.AddrComp = RADIO_ADDRESSCOMP_FILT_OFF;
            SX1280.PacketParams.Params.Gfsk.HeaderType = ( fixLen == true ) ? RADIO_PACKET_FIXED_LENGTH : RADIO_PACKET_VARIABLE_LENGTH;

            if( crcOn == true )
            {
                SX1280.PacketParams.Params.Gfsk.CrcLength = RADIO_CRC_2_BYTES_CCIT;
            }
            else
            {
                SX1280.PacketParams.Params.Gfsk.CrcLength = RADIO_CRC_OFF;
            }
            SX1280.PacketParams.Params.Gfsk.DcFree = GfskWhitening;

            RadioStandby( );
            RadioSetModem( MODEM_FSK );
            SX1280SetModulationParams( &SX1280.ModulationParams );
            SX1280SetPacketParams( &SX1280.PacketParams );
            SX1280SetSyncWord( GfskSyncWord );
            SX1280SetWhiteningSeed( 0x01FF );
            break;

        case MODEM_LORA:
            SX1280.ModulationParams.PacketType = PACKET_TYPE_LORA;
            SX1280.ModulationParams.Params.LoRa.SpreadingFactor = ( RadioLoRaSpreadingFactors_t )(datarate << 4);
            SX1280.ModulationParams.Params.LoRa.Bandwidth =  Bandwidths[bandwidth];
            SX1280.ModulationParams.Params.LoRa.CodingRate= ( RadioLoRaCodingRates_t )coderate;
            SX1280.ModulationParams.Params.LoRa.LowDatarateOptimize = 0x00;

            SX1280.PacketParams.PacketType = PACKET_TYPE_LORA;

            // NOTE: do not increase preamble length to 12 for SF5 and SF6 if it is below 12

            SX1280.PacketParams.Params.LoRa.PreambleLength = preambleLen;

            /* Bool cast to enum yielded 0x01 for fixed-length, but SX1280
             * expects 0x80 (datasheet 14.4.3). Use proper enum names. */
            SX1280.PacketParams.Params.LoRa.HeaderType = ( fixLen == true ) ? LORA_PACKET_FIXED_LENGTH : LORA_PACKET_VARIABLE_LENGTH;
            SX1280.PacketParams.Params.LoRa.PayloadLength = MaxPayloadLength;
            SX1280.PacketParams.Params.LoRa.CrcMode = ( RadioLoRaCrcModes_t )crcOn;
            SX1280.PacketParams.Params.LoRa.InvertIQ = ( RadioLoRaIQModes_t )iqInverted;

            RadioStandby( );
            RadioSetModem( MODEM_LORA );
            SX1280SetModulationParams( &SX1280.ModulationParams );
            SX1280SetPacketParams( &SX1280.PacketParams );
            break;
    }

    SX1280SetRfTxPower( power );
}

bool RadioCheckRfFrequency( uint32_t frequency )
{
    return (frequency >= 2400000000UL) && (frequency <= 2500000000UL);
}

static uint32_t RadioGetLoRaBandwidthInHz( RadioLoRaBandwidths_t bw )
{
    uint32_t bandwidthInHz = 0;

    switch( bw )
    {
    case LORA_BW_0200:
        bandwidthInHz = 203125UL;
        break;
    case LORA_BW_0400:
        bandwidthInHz = 406250UL;
        break;
    case LORA_BW_0800:
        bandwidthInHz = 812500UL;
        break;
    case LORA_BW_1600:
        bandwidthInHz = 1625000UL;
        break;
    }

    return bandwidthInHz;
}

static uint32_t RadioGetGfskTimeOnAirNumerator( uint32_t datarate, uint8_t coderate,
                              uint16_t preambleLen, bool fixLen, uint8_t payloadLen,
                              bool crcOn )
{
    const RadioAddressComp_t addrComp = RADIO_ADDRESSCOMP_FILT_OFF;

    return ( preambleLen << 3 ) +
           ( ( fixLen == false ) ? 8 : 0 ) +
             ( GfskSyncWordLength << 3 ) +
             ( ( payloadLen +
               ( addrComp == RADIO_ADDRESSCOMP_FILT_OFF ? 0 : 1 ) +
               ( ( crcOn == true ) ? 2 : 0 )
               ) << 3
             );
}

static uint32_t RadioGetLoRaTimeOnAirNumerator( uint32_t bandwidth,
                              uint32_t datarate, uint8_t coderate,
                              uint16_t preambleLen, bool fixLen, uint8_t payloadLen,
                              bool crcOn )
{
    int32_t crDenom           = coderate + 4;
    // // Ensure that the preamble length is at least 12 symbols when using SF5 or
    // // SF6
    // if( ( datarate == 5 ) || ( datarate == 6 ) )
    // {
    //     if( preambleLen < 12 )
    //     {
    //         preambleLen = 12;
    //     }
    // }

    int32_t ceilDenominator;
    int32_t ceilNumerator = ( payloadLen << 3 ) +
                            ( crcOn ? 16 : 0 ) -
                            ( 4 * datarate ) +
                            ( fixLen ? 0 : 20 );

    if( datarate <= 6 )
    {
        ceilDenominator = 4 * datarate;
    }
    else
    {
        ceilNumerator += 8;

        ceilDenominator = 4 * datarate;
    }

    if( ceilNumerator < 0 )
    {
        ceilNumerator = 0;
    }

    // Perform integral ceil()
    int32_t intermediate =
        ( ( ceilNumerator + ceilDenominator - 1 ) / ceilDenominator ) * crDenom + preambleLen + 12;

    if( datarate <= 6 )
    {
        intermediate += 2;
    }

    return ( uint32_t )( ( 4 * intermediate + 1 ) * ( 1 << ( datarate - 2 ) ) );
}

uint32_t RadioTimeOnAir( RadioModems_t modem, uint32_t bandwidth,
                              uint32_t datarate, uint8_t coderate,
                              uint16_t preambleLen, bool fixLen, uint8_t payloadLen,
                              bool crcOn )
{
    // NOTE: modified in order to return us instead of ms!
    uint64_t numerator = 0;
    uint32_t denominator = 1;

    switch( modem )
    {
    case MODEM_FSK:
        {
            numerator   = 1000000U * RadioGetGfskTimeOnAirNumerator( datarate, coderate,
                                                                  preambleLen, fixLen,
                                                                  payloadLen, crcOn );
            denominator = datarate;
        }
        break;
    case MODEM_LORA:
        {
            numerator   = 1000000U * ((uint64_t) RadioGetLoRaTimeOnAirNumerator( bandwidth, datarate,
                                                                  coderate, preambleLen,
                                                                  fixLen, payloadLen, crcOn ));
            denominator = RadioGetLoRaBandwidthInHz( Bandwidths[bandwidth] );
        }
        break;
    }
    // Perform integral ceil()
    return ( numerator + denominator - 1 ) / denominator;
}

void RadioSendPayload( uint8_t *buffer, uint8_t size )
{
    uint16_t mask = IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT;
    RadioSendPayloadMask(mask, buffer, size);
}

void RadioSendPayloadMask( uint16_t mask, uint8_t *buffer, uint8_t size )
{
    SX1280SetDioIrqParams( mask,
                           mask,
                           IRQ_RADIO_NONE,
                           IRQ_RADIO_NONE );

    if( SX1280GetPacketType( ) == PACKET_TYPE_LORA )
    {
        SX1280.PacketParams.Params.LoRa.PayloadLength = size;
    }
    else
    {
        SX1280.PacketParams.Params.Gfsk.PayloadLength = size;
    }
    SX1280SetPacketParams( &SX1280.PacketParams );
    SX1280SendPayload( buffer, size, 0 );
}

void RadioSleep( void )
{
    SleepParams_t params = { 0 };

    params.Fields.DataRetention = 1;
    SX1280SetSleep( params );

    delay_us(600); // at least 500us required to put radio into sleep state (see datasheet p.67)
}

void RadioColdSleep( void )
{
    SleepParams_t params = { 0 };

    params.Fields.DataRetention = 0;
    SX1280SetSleep( params );

    delay_us(600); // at least 500us required to put radio into sleep state (see datasheet p.67)
}

void RadioStandby( void )
{
    SX1280SetStandby( STDBY_XOSC );

    if (RADIO_READ_DIO1_PIN())
    {
        // there is still a pending interrupt: clear it
        SX1280ClearIrqStatus( IRQ_RADIO_ALL );
        IrqFired = false;
    }
}

/* SX1280 SetRx/SetTx 3-byte timeout encoding:
 *   buf[0] = PeriodBase  (0=15.625us, 1=62.5us, 2=1ms, 3=4ms)
 *   buf[1] = Count MSB,  buf[2] = Count LSB
 *   Count=0xFFFF + valid PeriodBase = continuous; Count=0 = no timeout.
 * Previous code did `timeout_ms <<= 6` and 0xFFFFFF for continuous, which
 * planted INVALID PeriodBase (0x0E..0xFF) into byte 0 — chip rejected or
 * mis-interpreted the command. This was the reason LWB never RX'd on
 * SX1280 (gfsk_test worked only because it hand-built {0x00,0xFF,0xFF}). */
static uint32_t SX1280_FormatRxTxTimeout( uint32_t timeout_ms, bool continuous )
{
    /* SX1280 SetRx Count semantics (Semtech header, sx1280.h):
     *   RX_TX_CONTINUOUS = { Step=0, NbSteps=0xFFFF }  -> stay in RX until aborted
     *   RX_TX_SINGLE     = { Step=0, NbSteps=0     }  -> exit on FIRST RX event
     *                                                    (incl. sync error)
     * Gloria calls radio_receive(timeout_ms=0) expecting "wait indefinitely
     * until I call radio_standby()". Returning 0 here put the chip in single
     * shot mode, so NODE exited RX on the first noise-triggered sync error
     * and never saw the HOST schedule packet -> permanent LWB timeout. */
    if( continuous || timeout_ms == 0 ) {
        return 0x00FFFFu;  /* PeriodBase=0, Count=0xFFFF -> continuous */
    }
    if( timeout_ms > 65535u ) timeout_ms = 65535u;
    return ( 0x02u << 16 ) | timeout_ms; /* PeriodBase=1ms, Count=timeout_ms */
}

void RadioRx( uint32_t timeout_ms, bool continuous, bool scheduled )
{
    SX1280SetRx( SX1280_FormatRxTxTimeout( timeout_ms, continuous ), !scheduled, false );
}

void RadioRxMask( uint16_t mask, uint32_t timeout_ms, bool continuous, bool scheduled )
{
    SX1280SetDioIrqParams( mask,
                           mask,
                           IRQ_RADIO_NONE,
                           IRQ_RADIO_NONE );
    RadioRx( timeout_ms, continuous, scheduled );
}

void RadioRxBoosted( uint32_t timeout_ms, bool continuous, bool scheduled )
{
    SX1280SetRx( SX1280_FormatRxTxTimeout( timeout_ms, continuous ), !scheduled, true );
}

void RadioRxBoostedMask( uint16_t mask, uint32_t timeout_ms, bool continuous, bool scheduled )
{
    SX1280SetDioIrqParams( mask,
                           mask,
                           IRQ_RADIO_NONE,
                           IRQ_RADIO_NONE );
    RadioRxBoosted( timeout_ms, continuous, scheduled );
}

void RadioSetRxDutyCycle( uint32_t rxTime, uint32_t sleepTime, bool scheduled )
{
    SX1280SetRxDutyCycle( rxTime, sleepTime, !scheduled );
}

void RadioStartCad( void )
{
    SX1280SetDioIrqParams( IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED, IRQ_CAD_DONE | IRQ_CAD_ACTIVITY_DETECTED, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
    SX1280SetCad( );
}

void RadioTx( uint32_t timeout_ms, bool scheduled )
{
    /* Same SX1280 PeriodBase/Count format as RX. timeout_ms=0 → no timeout
     * (chip stays in TX until TxDone or external abort). */
    SX1280SetTx( SX1280_FormatRxTxTimeout( timeout_ms, false ), !scheduled );
}

void RadioTxMask( uint16_t mask, uint32_t timeout_ms, bool scheduled )
{
    SX1280SetDioIrqParams( mask,
                           mask,
                           IRQ_RADIO_NONE,
                           IRQ_RADIO_NONE );

    RadioTx( timeout_ms, scheduled );
}

void RadioSetTxContinuousWave( uint32_t freq, int8_t power )
{
    SX1280SetRfFrequency( freq );
    SX1280SetRfTxPower( power );
    SX1280SetTxContinuousWave( );
}

int16_t RadioRssi( RadioModems_t modem )
{
    return SX1280GetRssiInst( );
}

void RadioWrite( uint16_t addr, uint8_t data )
{
    SX1280WriteRegister( addr, data );
}

uint8_t RadioRead( uint16_t addr )
{
    return SX1280ReadRegister( addr );
}

void RadioWriteBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{
    SX1280WriteRegisters( addr, buffer, size );
}

void RadioReadBuffer( uint16_t addr, uint8_t *buffer, uint8_t size )
{
    SX1280ReadRegisters( addr, buffer, size );
}

void RadioWriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1280WriteBuffer( 0, buffer, size );
}

void RadioReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1280ReadBuffer( 0, buffer, size );
}

void RadioSetMaxPayloadLength( uint8_t max )
{
    if( SX1280GetPacketType( ) == PACKET_TYPE_LORA )
    {
        SX1280.PacketParams.Params.LoRa.PayloadLength = MaxPayloadLength = max;
        SX1280SetPacketParams( &SX1280.PacketParams );
    }
    else
    {
        if( SX1280.PacketParams.Params.Gfsk.HeaderType == RADIO_PACKET_VARIABLE_LENGTH )
        {
            SX1280.PacketParams.Params.Gfsk.PayloadLength = MaxPayloadLength = max;
            SX1280SetPacketParams( &SX1280.PacketParams );
        }
    }
}

void RadioSetGfskWhitening( uint8_t whitening )
{
    SX1280.PacketParams.Params.Gfsk.DcFree = GfskWhitening = whitening;
    SX1280SetPacketParams( &SX1280.PacketParams );
}

void RadioSetGfskSyncWordLength( uint8_t syncWordLength )
{
    SX1280.PacketParams.Params.Gfsk.SyncWordLength = GfskSyncWordLength = syncWordLength;
    SX1280SetPacketParams( &SX1280.PacketParams );
}

void RadioSetGfskSyncWord( uint8_t syncWord[8] )
{
    uint32_t i;
    for (i = 0U; i < 8U; i++)
    {
        GfskSyncWord[i] = syncWord[i];
    }
    SX1280SetSyncWord( GfskSyncWord );
}

void RadioSetPublicNetwork( bool enable )
{
    RadioPublicNetwork.Current = RadioPublicNetwork.Previous = enable;

    RadioSetModem( MODEM_LORA );
    if( enable == true )
    {
        // Change LoRa modem SyncWord
        SX1280WriteRegister( REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD );
    }
    else
    {
        // Change LoRa modem SyncWord
        SX1280WriteRegister( REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD );
    }
}

uint32_t RadioGetWakeupTime( void )
{
    return RADIO_WAKEUP_TIME;
}

void RadioOnTxTimeoutIrq( void )
{
    if( ( RadioEvents != NULL ) && ( RadioEvents->TxTimeout != NULL ) )
    {
        RadioEvents->TxTimeout( );
    }
}

void RadioOnRxTimeoutIrq( void )
{
    if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
    {
        RadioEvents->RxTimeout( false );
    }
}

void RadioOnDioIrq( void )
{
    IrqFired = true;
}

void RadioIrqProcess( void )
{
    if( IrqFired == true )
    {
        IrqFired = false;
        uint16_t irqRegs = SX1280GetIrqStatus( );
        SX1280ClearIrqStatus( irqRegs );    // takes ~20us

        // if DIO1 is still high, then most likely another radio interrupt has just occurred -> read registers again
        while (RADIO_READ_DIO1_PIN())
        {
            uint16_t irqRegs2 = SX1280GetIrqStatus( );
            if (!irqRegs2)
            {
                break;    // most likely the read command failed -> abort
            }
            SX1280ClearIrqStatus( irqRegs2 );
            irqRegs |= irqRegs2;
        }

        if( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
        {
            SX1280SetOperatingMode( MODE_STDBY_XOSC );
            if( ( RadioEvents != NULL ) && ( RadioEvents->TxDone != NULL ) )
            {
                RadioEvents->TxDone( );
            }
        }

        if( ( irqRegs & IRQ_SYNCWORD_VALID ) == IRQ_SYNCWORD_VALID )
        {
            if( ( RadioEvents != NULL ) && ( RadioEvents->RxSync != NULL ) )
            {
                RadioEvents->RxSync( );
            }
        }

        if( ( irqRegs & IRQ_HEADER_VALID ) == IRQ_HEADER_VALID )
        {
            if( ( RadioEvents != NULL ) && ( RadioEvents->RxSync != NULL ) )
            {
                RadioEvents->RxSync( );
            }
        }

        if( ( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
        {
            static PacketStatus_t RadioPktStatus;
            bool crc_error = (irqRegs & IRQ_CRC_ERROR) == IRQ_CRC_ERROR || (irqRegs & IRQ_HEADER_ERROR) == IRQ_HEADER_ERROR;
            uint8_t size;
            SX1280GetPayload( RadioRxPayload, &size, RADIO_MAX_PAYLOAD_SIZE );
            SX1280GetPacketStatus( &RadioPktStatus );
            if (SX1280GetOperatingMode() != MODE_RX_CONTINUOUS)
            {
                SX1280SetOperatingMode( MODE_STDBY_XOSC );
            }
            if( ( RadioEvents != NULL ) && ( RadioEvents->RxDone != NULL ) )
            {
                if (RadioPktStatus.packetType == PACKET_TYPE_LORA) {
                    RadioEvents->RxDone( RadioRxPayload, size, RadioPktStatus.Params.LoRa.RssiPkt, RadioPktStatus.Params.LoRa.SnrPkt, crc_error );
                }
                else {
                    RadioEvents->RxDone( RadioRxPayload, size, RadioPktStatus.Params.Gfsk.RssiAvg, 0, crc_error );
                }
            }
        }

        if( ( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
        {
            if( ( irqRegs & IRQ_RX_DONE ) != IRQ_RX_DONE )
            {
                if (SX1280GetOperatingMode() != MODE_RX_CONTINUOUS)
                {
                    SX1280SetOperatingMode( MODE_STDBY_XOSC );
                }
                if( ( RadioEvents != NULL ) && ( RadioEvents->RxError != NULL ) )
                {
                    RadioEvents->RxError( );
                }
            }
        }

        if( ( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
        {
            if( ( irqRegs & IRQ_RX_DONE ) != IRQ_RX_DONE )
            {
                if (SX1280GetOperatingMode() != MODE_RX_CONTINUOUS)
                {
                    SX1280SetOperatingMode( MODE_STDBY_XOSC );
                }
                if( ( RadioEvents != NULL ) && ( RadioEvents->RxError != NULL ) )
                {
                    RadioEvents->RxError( );
                }
            }
        }

        if( ( irqRegs & IRQ_CAD_DONE ) == IRQ_CAD_DONE )
        {
            SX1280SetOperatingMode( MODE_STDBY_XOSC );
            if( ( RadioEvents != NULL ) && ( RadioEvents->CadDone != NULL ) )
            {
                RadioEvents->CadDone( ( ( irqRegs & IRQ_CAD_ACTIVITY_DETECTED ) == IRQ_CAD_ACTIVITY_DETECTED ) );
            }
        }

        if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
        {
            if( SX1280GetOperatingMode( ) == MODE_TX )
            {
                SX1280SetOperatingMode( MODE_STDBY_XOSC );
                if( ( RadioEvents != NULL ) && ( RadioEvents->TxTimeout != NULL ) )
                {
                    RadioEvents->TxTimeout( );
                }
            }
            else if( SX1280GetOperatingMode( ) == MODE_RX )
            {
                SX1280SetOperatingMode( MODE_STDBY_XOSC );
                if( ( RadioEvents != NULL ) && ( RadioEvents->RxTimeout != NULL ) )
                {
                    RadioEvents->RxTimeout( );
                }
            }
        }

        if( ( irqRegs & IRQ_PREAMBLE_DETECTED ) == IRQ_PREAMBLE_DETECTED )
        {
            if( ( RadioEvents != NULL ) && ( RadioEvents->RxPreamble != NULL ) )
            {
                RadioEvents->RxPreamble( );
            }
        }
    }
}

#endif /* RADIO_ENABLE */
