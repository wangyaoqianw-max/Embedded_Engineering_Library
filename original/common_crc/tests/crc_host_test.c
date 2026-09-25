/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file s04_crc_host_test.c
 * @brief S04 CRC Common Host Test
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "crc.h"

#define TEST_VECTOR_TEXT                 "123456789"
#define TEST_VECTOR_LENGTH               (9U)
#define TEST_CRC8_SMBUS_EXPECTED         (0xF4U)
#define TEST_CRC16_XMODEM_EXPECTED       (0x31C3U)
#define TEST_CRC32_ISO_HDLC_EXPECTED     (0xCBF43926UL)

static int test_crc8_smbus(void)
{
    crc8_smbus_context_t context;
    uint8_t oneShot;
    uint8_t streaming;

    oneShot = crc8_smbus_calculate((const uint8_t *)TEST_VECTOR_TEXT, TEST_VECTOR_LENGTH);

    crc8_smbus_init(&context);
    crc8_smbus_update(&context, (const uint8_t *)TEST_VECTOR_TEXT, 3U);
    crc8_smbus_update(&context, (const uint8_t *)&TEST_VECTOR_TEXT[3], 2U);
    crc8_smbus_update(&context, (const uint8_t *)&TEST_VECTOR_TEXT[5], 4U);
    streaming = crc8_smbus_finalize(&context);

    return ((oneShot == TEST_CRC8_SMBUS_EXPECTED) && (streaming == TEST_CRC8_SMBUS_EXPECTED)) ? 0 : 1;
}

static int test_crc16_xmodem(void)
{
    crc16_xmodem_context_t context;
    uint16_t oneShot;
    uint16_t streaming;

    oneShot = crc16_xmodem_calculate((const uint8_t *)TEST_VECTOR_TEXT, TEST_VECTOR_LENGTH);

    crc16_xmodem_init(&context);
    crc16_xmodem_update(&context, (const uint8_t *)TEST_VECTOR_TEXT, 3U);
    crc16_xmodem_update(&context, (const uint8_t *)&TEST_VECTOR_TEXT[3], 2U);
    crc16_xmodem_update(&context, (const uint8_t *)&TEST_VECTOR_TEXT[5], 4U);
    streaming = crc16_xmodem_finalize(&context);

    return ((oneShot == TEST_CRC16_XMODEM_EXPECTED) && (streaming == TEST_CRC16_XMODEM_EXPECTED)) ? 0 : 1;
}

static int test_crc32_iso_hdlc(void)
{
    crc32_iso_hdlc_context_t context;
    uint32_t oneShot;
    uint32_t streaming;

    oneShot = crc32_iso_hdlc_calculate((const uint8_t *)TEST_VECTOR_TEXT, TEST_VECTOR_LENGTH);

    crc32_iso_hdlc_init(&context);
    crc32_iso_hdlc_update(&context, (const uint8_t *)TEST_VECTOR_TEXT, 3U);
    crc32_iso_hdlc_update(&context, (const uint8_t *)&TEST_VECTOR_TEXT[3], 2U);
    crc32_iso_hdlc_update(&context, (const uint8_t *)&TEST_VECTOR_TEXT[5], 4U);
    streaming = crc32_iso_hdlc_finalize(&context);

    return ((oneShot == TEST_CRC32_ISO_HDLC_EXPECTED) && (streaming == TEST_CRC32_ISO_HDLC_EXPECTED)) ? 0 : 1;
}

int main(void)
{
    if (test_crc8_smbus() != 0) {
        (void)printf("CRC-8/SMBUS test failed.\n");
        return 1;
    }

    if (test_crc16_xmodem() != 0) {
        (void)printf("CRC-16/XMODEM test failed.\n");
        return 1;
    }

    if (test_crc32_iso_hdlc() != 0) {
        (void)printf("CRC-32/ISO-HDLC test failed.\n");
        return 1;
    }

    (void)printf("S04 CRC host test passed.\n");
    return 0;
}
