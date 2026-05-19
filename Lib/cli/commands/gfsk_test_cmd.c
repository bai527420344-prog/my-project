/*
 * gfsk_test_cmd.c - raw GFSK hardware test commands for SX1280
 *
 * Purpose: bypass LWB / Gloria / Radio.* abstractions. Talk to the SX1280
 * directly via SX1280WriteCommand / SX1280ReadCommand / SX1280WriteRegisters
 * / SX1280WriteBuffer using literal byte sequences taken from the official
 * Semtech L476RG PingPong demo for GFSK mode.
 *
 * Demo reference config:
 *   - BR/BW    = GFSK_BLE_BR_0_125_BW_0_3 (0xEF) = 125 kbps / 300 kHz
 *   - ModIndex = MOD_IND_1_00 (0x03) = mod_idx 1.0
 *   - Shape    = BT_1_0 (0x10) = Gaussian BT=1.0
 *   - Preamble = PREAMBLE_LENGTH_32_BITS (0x70)
 *   - Sync len = SYNC_WORD_LEN_5_B (0x08) = 5 bytes
 *   - SyncMatch= RADIO_SELECT_SYNCWORD_1 (0x10)
 *   - HdrType  = RADIO_PACKET_VARIABLE_LENGTH (0x20)
 *   - CRC      = RADIO_CRC_3_BYTES (0x30)
 *   - White    = WHITENING_ENABLE (0x00)
 *   - SyncWord = DD A0 96 69 DD at register 0x09CE
 *   - Freq     = 2.45 GHz
 *   - Power    = +13 dBm, ramp 2us
 *
 * Commands:
 *   gfsk_test tx     -> configure + send "PING" packet, wait for TxDone
 *   gfsk_test rx     -> configure + enter continuous Rx
 *   gfsk_test poll   -> read GetStatus + GetIrqStatus, decode and print
 *   gfsk_test status -> read GetStatus once
 *   gfsk_test stop   -> back to STDBY_RC
 */

#include "flora_lib.h"

#if CLI_ENABLE

static command_return_t gfsk_test_tx_handler(command_execution_t execution);
static command_return_t gfsk_test_rx_handler(command_execution_t execution);
static command_return_t gfsk_test_poll_handler(command_execution_t execution);
static command_return_t gfsk_test_status_handler(command_execution_t execution);
static command_return_t gfsk_test_stop_handler(command_execution_t execution);
static command_return_t gfsk_test_cw_handler(command_execution_t execution);
static command_return_t gfsk_test_dump_handler(command_execution_t execution);

/* SetRfFrequency expects PLL steps. SX1280 PLL step = 52e6 / 2^18.
 * For 2.45 GHz: (2450e6 * 2^18) / 52e6 = 12348826 = 0xBC7693 */
#define GFSK_TEST_FREQ_PLL  0x00BC7693UL

/* IRQ-capture counters updated by EXTI4 ISR (see gfsk_test_irq_cb).
 * The default radio ISR (RadioOnDioIrq) reads + CLEARS IRQ status on the
 * chip immediately when DIO1 asserts. That race lets the chip's IRQ
 * register go back to 0 before our 10-ms poll loop can read it.
 * We install a custom callback via radio_set_irq_callback() that just
 * bumps a counter — it does NOT clear the chip IRQ, so the chip register
 * stays latched and the poll loop sees TxDone/RxDone normally. */
static volatile uint32_t gfsk_test_irq_count = 0;

static void gfsk_test_irq_cb(void)
{
    gfsk_test_irq_count++;
    /* Intentionally do NOT call SX1280ClearIrqStatus here — the test
     * poll loop reads + clears via SPI. */
}

static void gfsk_test_configure_radio(void)
{
    /* Install our IRQ callback so the default RadioOnDioIrq (which clears
     * the chip's IRQ register) does NOT consume IRQs before our poll loop
     * sees them. */
    radio_set_irq_callback(gfsk_test_irq_cb);
    gfsk_test_irq_count = 0;

    /* Step 1: STDBY_RC (required by datasheet 14.1.1 before SetPacketType). */
    {
        uint8_t arg = STDBY_RC;
        SX1280WriteCommand(RADIO_SET_STANDBY, &arg, 1);
    }

    /* Step 2: SetPacketType(GFSK) */
    {
        uint8_t arg = PACKET_TYPE_GFSK;
        SX1280WriteCommand(RADIO_SET_PACKETTYPE, &arg, 1);
    }

    /* Step 3: SetRfFrequency(2.45 GHz) */
    {
        uint8_t buf[3] = {
            (uint8_t)((GFSK_TEST_FREQ_PLL >> 16) & 0xFF),
            (uint8_t)((GFSK_TEST_FREQ_PLL >>  8) & 0xFF),
            (uint8_t)((GFSK_TEST_FREQ_PLL >>  0) & 0xFF),
        };
        SX1280WriteCommand(RADIO_SET_RFFREQUENCY, buf, 3);
    }

    /* Step 4: SetBufferBaseAddress(tx=0x00, rx=0x00) */
    {
        uint8_t buf[2] = { 0x00, 0x00 };
        SX1280WriteCommand(RADIO_SET_BUFFERBASEADDRESS, buf, 2);
    }

    /* Step 5: SetModulationParams - demo bytes (3 bytes) */
    {
        uint8_t buf[3] = { 0xEF, 0x03, 0x10 };
        SX1280WriteCommand(RADIO_SET_MODULATIONPARAMS, buf, 3);
    }

    /* Step 6: SetPacketParams - 7 bytes
     *   buf[0] = PreambleLength       (0x80 = 64 bits, mantissa 8 / exp 0)
     *                                  doubled from 32 to give AGC more
     *                                  time to settle on weak signals.
     *   buf[1] = SyncWordLength       (0x08 = 5 bytes)
     *   buf[2] = SyncWordMatch        (0x10 = enable SyncWord 1).
     *                                  IMPORTANT: 0x00 here means NO sync
     *                                  word matching at all — RX will never
     *                                  declare SyncValid or RxDone. The
     *                                  Semtech header calls this "AddrComp"
     *                                  with FILT_OFF=0x00, but the chip's
     *                                  actual semantics for GFSK byte 2 are
     *                                  "which sync words to match" (datasheet
     *                                  14.5.2: 0x10=SW1, 0x20=SW2, 0x40=SW3).
     *   buf[3] = HeaderType           (0x00 = FIXED_LENGTH)
     *   buf[4] = PayloadLength        (5 bytes = "PING\0")
     *   buf[5] = CrcLength            (0x20 = 2-byte CRC)
     *   buf[6] = Whitening            (0x00 = enabled)
     */
    {
        /* Preamble 0x70 = 32 bits (validated stable); 64-bit (0x80 / 0xF0)
         * was experimented with but doesn't help much and combined with
         * LNA-boost register write corrupted chip state. */
        uint8_t buf[7] = { 0x70, 0x08, 0x10, 0x00, 0x05, 0x20, 0x00 };
        SX1280WriteCommand(RADIO_SET_PACKETPARAMS, buf, 7);
    }

    /* Step 7: SyncWord 1 register write (0x09CE = SyncAddress1, 5 bytes MSB-first) */
    {
        uint8_t sync_word[5] = { 0xDD, 0xA0, 0x96, 0x69, 0xDD };
        SX1280WriteRegisters(0x09CE, sync_word, 5);
    }

    /* Step 8: SetTxParams(power=+13dBm, ramp=2us)
     * power byte = power_dBm + 18 (datasheet) */
    {
        uint8_t buf[2] = { (uint8_t)(13 + 18), 0x00 /* RADIO_RAMP_02_US */ };
        SX1280WriteCommand(RADIO_SET_TXPARAMS, buf, 2);
    }

    /* Step 9: SetDioIrqParams - enable all IRQs, route all to DIO1 */
    {
        uint8_t buf[8] = {
            0xFF, 0xFF,   /* IRQ mask: all */
            0xFF, 0xFF,   /* DIO1 mask: all */
            0x00, 0x00,   /* DIO2 mask: none */
            0x00, 0x00,   /* DIO3 mask: none */
        };
        SX1280WriteCommand(RADIO_SET_DIOIRQPARAMS, buf, 8);
    }

    /* Step 10: Clear IRQ status */
    {
        uint8_t buf[2] = { 0xFF, 0xFF };
        SX1280WriteCommand(RADIO_CLR_IRQSTATUS, buf, 2);
    }
}

static void gfsk_test_log_irq(uint16_t irq)
{
    char buf[160];
    int n = snprintf(buf, sizeof(buf), "IRQ=0x%04X", irq);
    if (irq == 0) {
        snprintf(buf + n, sizeof(buf) - n, " (none)");
    } else {
        if (irq & (1u << 0))  n += snprintf(buf + n, sizeof(buf) - n, " TxDone");
        if (irq & (1u << 1))  n += snprintf(buf + n, sizeof(buf) - n, " RxDone");
        if (irq & (1u << 2))  n += snprintf(buf + n, sizeof(buf) - n, " SyncValid");
        if (irq & (1u << 3))  n += snprintf(buf + n, sizeof(buf) - n, " SyncError");
        if (irq & (1u << 4))  n += snprintf(buf + n, sizeof(buf) - n, " HeaderValid");
        if (irq & (1u << 5))  n += snprintf(buf + n, sizeof(buf) - n, " HeaderError");
        if (irq & (1u << 6))  n += snprintf(buf + n, sizeof(buf) - n, " CrcError");
        if (irq & (1u << 7))  n += snprintf(buf + n, sizeof(buf) - n, " RngSlaveReq");
        if (irq & (1u << 8))  n += snprintf(buf + n, sizeof(buf) - n, " CADDone");
        if (irq & (1u << 9))  n += snprintf(buf + n, sizeof(buf) - n, " CADDetected");
        if (irq & (1u << 10)) n += snprintf(buf + n, sizeof(buf) - n, " RxTxTimeout");
        if (irq & (1u << 11)) n += snprintf(buf + n, sizeof(buf) - n, " PreambleDet");
    }
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);
}

/* SX1280 status byte layout per SX1280 datasheet section 11.4
 * (NB: sx1280.h has a RadioStatus_u union whose bit field comments
 * disagree with the datasheet — datasheet wins; empirical evidence
 * confirms bits[7:5]=ChipMode, bits[4:2]=CmdStatus).
 *   bit  7    : Reserved
 *   bits 6:5  : ChipMode      Wait no — bit 7 reserved, bits 7:5 = ChipMode? */
static void gfsk_test_log_status(uint8_t status)
{
    uint8_t chip_mode = (status >> 5) & 0x07;
    uint8_t cmd_stat  = (status >> 2) & 0x07;
    const char* mode_str = "?";
    switch (chip_mode) {
        case 0x02: mode_str = "STDBY_RC";      break;
        case 0x03: mode_str = "STDBY_XOSC";    break;
        case 0x04: mode_str = "FS";            break;
        case 0x05: mode_str = "RX";            break;
        case 0x06: mode_str = "TX";            break;
    }
    const char* cmd_str = "?";
    switch (cmd_stat) {
        case 0x1: cmd_str = "ok";        break;
        case 0x2: cmd_str = "data_avail";break;
        case 0x3: cmd_str = "timeout";   break;
        case 0x4: cmd_str = "cmd_err";   break;
        case 0x5: cmd_str = "cmd_fail";  break;
    }
    char buf[96];
    snprintf(buf, sizeof(buf),
             "Status=0x%02X mode=%s(0x%X) cmd=%s(0x%X)",
             status, mode_str, chip_mode, cmd_str, cmd_stat);
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);
}

static command_return_t gfsk_test_tx_handler(command_execution_t execution)
{
    (void)execution;
    char buf[80];

    gfsk_test_configure_radio();

    /* Status right after config */
    cli_log_inline("after-config:", CLI_LOG_LEVEL_DEFAULT, true, true, true);
    uint8_t status_pre = 0;
    SX1280ReadCommand(RADIO_GET_STATUS, &status_pre, 1);
    gfsk_test_log_status(status_pre);

    /* Write payload to buffer at offset 0 */
    uint8_t payload[] = "PING";
    SX1280WriteBuffer(0x00, payload, sizeof(payload));

    /* SetTx(timeout=0): immediate TX, no timeout */
    bool tx_ok;
    {
        uint8_t tbuf[3] = { 0x00, 0x00, 0x00 };
        tx_ok = SX1280WriteCommand(RADIO_SET_TX, tbuf, 3);
    }
    snprintf(buf, sizeof(buf), "SetTx returned %s", tx_ok ? "OK" : "FAIL");
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* Status immediately after SetTx — should be MODE_TX(0x6) briefly.
     * With 5-byte fixed payload at 125kbps, TX is only ~1.1 ms, so by
     * the time we read here the chip may have already returned to
     * STDBY_RC. Either is fine — we mainly want to confirm the chip
     * transitioned at all and cmdstatus reports a valid result. */
    delay_us(200);  /* 200 us: catch the chip mid-TX if possible */
    cli_log_inline("post-SetTx (+200us):", CLI_LOG_LEVEL_DEFAULT, true, true, true);
    uint8_t status_post = 0;
    SX1280ReadCommand(RADIO_GET_STATUS, &status_post, 1);
    gfsk_test_log_status(status_post);

    /* Also read IRQ immediately — it's latched, so if TX hardware ever
     * completed at all, TxDone (bit 0) should be present here even if
     * mode has already returned to STDBY. */
    {
        uint8_t irq_buf[2] = { 0, 0 };
        SX1280ReadCommand(RADIO_GET_IRQSTATUS, irq_buf, 2);
        uint16_t irq = ((uint16_t)irq_buf[0] << 8) | irq_buf[1];
        snprintf(buf, sizeof(buf), "post-SetTx IRQ snapshot:");
        cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);
        gfsk_test_log_irq(irq);
    }

    cli_log_inline("polling IRQ for 500ms...",
                   CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* Poll IRQ for up to 500 ms */
    for (int i = 0; i < 50; i++) {
        delay_us(10000);   /* 10 ms */
        uint8_t irq_buf[2] = { 0, 0 };
        SX1280ReadCommand(RADIO_GET_IRQSTATUS, irq_buf, 2);
        uint16_t irq = ((uint16_t)irq_buf[0] << 8) | irq_buf[1];
        if (irq != 0) {
            snprintf(buf, sizeof(buf), "after %d ms (exti hits=%lu):",
                     (i + 1) * 10, (unsigned long)gfsk_test_irq_count);
            cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);
            gfsk_test_log_irq(irq);
            /* Clear */
            uint8_t clr[2] = { 0xFF, 0xFF };
            SX1280WriteCommand(RADIO_CLR_IRQSTATUS, clr, 2);
            return CMD_RET_SUCCESS;
        }
    }
    snprintf(buf, sizeof(buf), "TX poll timeout (no IRQ in 500ms, exti hits=%lu)",
             (unsigned long)gfsk_test_irq_count);
    cli_log_inline(buf, CLI_LOG_LEVEL_WARNING, true, true, true);
    return CMD_RET_SUCCESS;
}

static command_return_t gfsk_test_rx_handler(command_execution_t execution)
{
    (void)execution;
    char buf[80];

    gfsk_test_configure_radio();

    /* SetRx continuous: PeriodBase=0x00, Count=0xFFFF. */
    {
        uint8_t tbuf[3] = { 0x00, 0xFF, 0xFF };
        SX1280WriteCommand(RADIO_SET_RX, tbuf, 3);
    }

    cli_log_inline("GFSK RX entered. 5 RSSI samples then idle — use 'gfsk_test poll' to read IRQ.",
                   CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* Simple 5 samples at 20ms intervals to verify chip is in RX */
    delay_us(20000);
    for (int i = 0; i < 5; i++) {
        uint8_t status = 0, rssi_raw = 0;
        SX1280ReadCommand(RADIO_GET_STATUS, &status, 1);
        SX1280ReadCommand(RADIO_GET_RSSIINST, &rssi_raw, 1);
        uint8_t mode = (status >> 5) & 0x07;
        snprintf(buf, sizeof(buf),
                 "  +%dms: mode=0x%X RSSI=%d dBm",
                 (i + 1) * 20, mode, -((int)rssi_raw) / 2);
        cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);
        delay_us(20000);
    }
    return CMD_RET_SUCCESS;
}

static command_return_t gfsk_test_poll_handler(command_execution_t execution)
{
    (void)execution;
    char dbg[80];

    /* Read EVERYTHING first into locals, only print after — this way
     * if cli_log_inline gets stuck partway, we still get all the values
     * in subsequent prints (or via debugger). */
    uint32_t exti      = gfsk_test_irq_count;
    uint8_t  status    = 0;
    uint8_t  rssi_raw  = 0;
    uint8_t  irq_buf[2] = { 0, 0 };

    SX1280ReadCommand(RADIO_GET_STATUS,    &status,    1);
    SX1280ReadCommand(RADIO_GET_RSSIINST,  &rssi_raw,  1);
    SX1280ReadCommand(RADIO_GET_IRQSTATUS, irq_buf,    2);
    uint16_t irq = ((uint16_t)irq_buf[0] << 8) | irq_buf[1];

    /* ONE compact line with everything important — this is the line we
     * absolutely need to see. */
    snprintf(dbg, sizeof(dbg),
             "POLL: exti=%lu Status=0x%02X RSSI=0x%02X IRQ=0x%04X",
             (unsigned long)exti, status, rssi_raw, irq);
    cli_log_inline(dbg, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* Then optionally the decoded view */
    gfsk_test_log_status(status);
    gfsk_test_log_irq(irq);

    /* If RxDone (bit 1) was latched, read payload. Small stack (32B). */
    if (irq & (1u << 1)) {
        uint8_t rxbuf_status[2] = { 0, 0 };
        SX1280ReadCommand(RADIO_GET_RXBUFFERSTATUS, rxbuf_status, 2);
        uint8_t len    = rxbuf_status[0];
        uint8_t offset = rxbuf_status[1];
        snprintf(dbg, sizeof(dbg), "RxBuffer: len=%u offset=0x%02X", len, offset);
        cli_log_inline(dbg, CLI_LOG_LEVEL_DEFAULT, true, true, true);
        if (len > 0 && len <= 16) {
            uint8_t payload[16] = { 0 };
            SX1280ReadBuffer(offset, payload, len);
            int n = snprintf(dbg, sizeof(dbg), "Payload[%u]=", len);
            for (int i = 0; i < len && n < (int)sizeof(dbg) - 4; i++) {
                n += snprintf(dbg + n, sizeof(dbg) - n, "%02X", payload[i]);
            }
            cli_log_inline(dbg, CLI_LOG_LEVEL_DEFAULT, true, true, true);
        }
    }

    /* Clear IRQ */
    uint8_t clr[2] = { 0xFF, 0xFF };
    SX1280WriteCommand(RADIO_CLR_IRQSTATUS, clr, 2);
    return CMD_RET_SUCCESS;
}

static command_return_t gfsk_test_status_handler(command_execution_t execution)
{
    (void)execution;
    uint8_t status = 0;
    SX1280ReadCommand(RADIO_GET_STATUS, &status, 1);
    gfsk_test_log_status(status);
    return CMD_RET_SUCCESS;
}

static command_return_t gfsk_test_stop_handler(command_execution_t execution)
{
    (void)execution;
    uint8_t arg = STDBY_RC;
    SX1280WriteCommand(RADIO_SET_STANDBY, &arg, 1);
    cli_log_inline("Radio set to STDBY_RC", CLI_LOG_LEVEL_DEFAULT, true, true, true);
    return CMD_RET_SUCCESS;
}

/* SetTxContinuousWave (opcode 0xD1, no args). Pure carrier emission,
 * bypasses all packet handling. If chip can't even enter MODE_TX for CW,
 * the TX hardware is fundamentally broken on this silicon. */
static command_return_t gfsk_test_cw_handler(command_execution_t execution)
{
    (void)execution;
    char buf[80];

    gfsk_test_configure_radio();

    /* SetTxContinuousWave: opcode 0xD1, no args */
    bool ok;
    {
        uint8_t dummy = 0;
        ok = SX1280WriteCommand((RadioCommands_t)0xD1, &dummy, 0);
    }
    snprintf(buf, sizeof(buf), "SetTxContinuousWave returned %s", ok ? "OK" : "FAIL");
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* Check status at multiple intervals */
    for (int i = 0; i < 5; i++) {
        delay_us(10000);    /* 10 ms */
        uint8_t status = 0;
        SX1280ReadCommand(RADIO_GET_STATUS, &status, 1);
        uint8_t mode = (status >> 5) & 0x07;
        snprintf(buf, sizeof(buf), "  +%d ms: Status=0x%02X mode=0x%X (%s)",
                 (i + 1) * 10, status, mode,
                 mode == 0x06 ? "TX" :
                 mode == 0x02 ? "STDBY_RC" :
                 mode == 0x03 ? "STDBY_XOSC" : "?");
        cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);
    }
    cli_log_inline("(CW continues. Use 'gfsk_test stop' to exit.)",
                   CLI_LOG_LEVEL_DEFAULT, true, true, true);
    return CMD_RET_SUCCESS;
}

/* Read back the GFSK config registers we wrote, to verify our writes
 * actually took effect. */
static command_return_t gfsk_test_dump_handler(command_execution_t execution)
{
    (void)execution;
    char buf[160];

    /* Modulation params: chip exposes via RADIO_GET_PACKETSTATUS? No, those
     * are only readable via the original SetModulationParams write. Skip.
     * But we can read several GFSK config registers directly. */

    /* SyncWord 1 at 0x09CE (5 bytes) */
    uint8_t sync[5] = { 0 };
    SX1280ReadRegisters(0x09CE, sync, 5);
    snprintf(buf, sizeof(buf), "SyncWord1[5]=%02X %02X %02X %02X %02X",
             sync[0], sync[1], sync[2], sync[3], sync[4]);
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* Whitening seed at 0x09C5 */
    uint8_t whit = 0;
    SX1280ReadRegisters(0x09C5, &whit, 1);
    snprintf(buf, sizeof(buf), "WhiteningSeed[0x09C5]=0x%02X", whit);
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* CRC poly 0x09C6-0x09C7 */
    uint8_t crc_poly[2] = { 0 };
    SX1280ReadRegisters(0x09C6, crc_poly, 2);
    snprintf(buf, sizeof(buf), "CrcPoly[0x09C6]=%02X %02X",
             crc_poly[0], crc_poly[1]);
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* Sync Address Control 0x09CD (sync match tolerance, bit 7 reset=0x80) */
    uint8_t sync_ctrl = 0;
    SX1280ReadRegisters(0x09CD, &sync_ctrl, 1);
    snprintf(buf, sizeof(buf), "SyncAddrCtrl[0x09CD]=0x%02X", sync_ctrl);
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* Packet Preamble Settings 0x09C1 */
    uint8_t pkt_pre = 0;
    SX1280ReadRegisters(0x09C1, &pkt_pre, 1);
    snprintf(buf, sizeof(buf), "PktPreamble[0x09C1]=0x%02X", pkt_pre);
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    /* FW version 0x0153 */
    uint8_t fw[2] = { 0 };
    SX1280ReadRegisters(0x0153, fw, 2);
    snprintf(buf, sizeof(buf), "FwVersion[0x0153]=0x%02X%02X", fw[0], fw[1]);
    cli_log_inline(buf, CLI_LOG_LEVEL_DEFAULT, true, true, true);

    return CMD_RET_SUCCESS;
}

/* --- Command registration --- */

static command_t gfsk_test_tx_command = {
    .execution_ptr = &gfsk_test_tx_handler,
    .name = "tx",
    .description = "Configure GFSK and send one 'PING' packet (demo config: 125k/300k mod_idx=1)",
    .prompt = "",
    .parameters = NULL,
    .parameter_count = 0,
    .children = NULL,
    .children_count = 0,
    .executable = true,
    .built_in = false,
    .has_prompt = false,
    .hidden = false,
    .children_only_in_prompt_invokable = false,
};

static command_t gfsk_test_rx_command = {
    .execution_ptr = &gfsk_test_rx_handler,
    .name = "rx",
    .description = "Configure GFSK and enter continuous RX",
    .prompt = "",
    .parameters = NULL,
    .parameter_count = 0,
    .children = NULL,
    .children_count = 0,
    .executable = true,
    .built_in = false,
    .has_prompt = false,
    .hidden = false,
    .children_only_in_prompt_invokable = false,
};

static command_t gfsk_test_poll_command = {
    .execution_ptr = &gfsk_test_poll_handler,
    .name = "poll",
    .description = "Read chip status + IRQ register, decode and print",
    .prompt = "",
    .parameters = NULL,
    .parameter_count = 0,
    .children = NULL,
    .children_count = 0,
    .executable = true,
    .built_in = false,
    .has_prompt = false,
    .hidden = false,
    .children_only_in_prompt_invokable = false,
};

static command_t gfsk_test_status_command = {
    .execution_ptr = &gfsk_test_status_handler,
    .name = "status",
    .description = "Read GetStatus, decode chip mode",
    .prompt = "",
    .parameters = NULL,
    .parameter_count = 0,
    .children = NULL,
    .children_count = 0,
    .executable = true,
    .built_in = false,
    .has_prompt = false,
    .hidden = false,
    .children_only_in_prompt_invokable = false,
};

static command_t gfsk_test_stop_command = {
    .execution_ptr = &gfsk_test_stop_handler,
    .name = "stop",
    .description = "Force radio to STDBY_RC",
    .prompt = "",
    .parameters = NULL,
    .parameter_count = 0,
    .children = NULL,
    .children_count = 0,
    .executable = true,
    .built_in = false,
    .has_prompt = false,
    .hidden = false,
    .children_only_in_prompt_invokable = false,
};

static command_t gfsk_test_cw_command = {
    .execution_ptr = &gfsk_test_cw_handler,
    .name = "cw",
    .description = "Issue SetTxContinuousWave (0xD1), poll chip mode 5x10ms",
    .prompt = "",
    .parameters = NULL,
    .parameter_count = 0,
    .children = NULL,
    .children_count = 0,
    .executable = true,
    .built_in = false,
    .has_prompt = false,
    .hidden = false,
    .children_only_in_prompt_invokable = false,
};

static command_t gfsk_test_dump_command = {
    .execution_ptr = &gfsk_test_dump_handler,
    .name = "dump",
    .description = "Read back GFSK config registers (sync word, CRC poly, etc.) and FW version",
    .prompt = "",
    .parameters = NULL,
    .parameter_count = 0,
    .children = NULL,
    .children_count = 0,
    .executable = true,
    .built_in = false,
    .has_prompt = false,
    .hidden = false,
    .children_only_in_prompt_invokable = false,
};

static command_t* gfsk_test_subcommands[] = {
    &gfsk_test_tx_command,
    &gfsk_test_rx_command,
    &gfsk_test_poll_command,
    &gfsk_test_status_command,
    &gfsk_test_stop_command,
    &gfsk_test_cw_command,
    &gfsk_test_dump_command,
};

static command_t gfsk_test_command = {
    .execution_ptr = NULL,
    .name = "gfsk_test",
    .description = "Raw SX1280 GFSK hardware test (bypasses LWB / Radio.* abstractions)",
    .prompt = "",
    .parameters = NULL,
    .parameter_count = 0,
    .children = NULL,
    .children_count = 0,
    .executable = false,
    .built_in = false,
    .has_prompt = false,
    .hidden = false,
    .children_only_in_prompt_invokable = false,
};

static command_t* gfsk_test_commands[] = { &gfsk_test_command };

void gfsk_test_register_commands(void)
{
    command_register(&gfsk_test_command, gfsk_test_subcommands,
                     COMMAND_COUNT(gfsk_test_subcommands));
    command_register(NULL, gfsk_test_commands, 1);
}

#endif /* CLI_ENABLE */
