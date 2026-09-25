/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file crc.c
 * @brief CRC Common 软件 bitwise 实现
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "crc.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define CRC8_SMBUS_POLYNOMIAL             (0x07U)
#define CRC16_XMODEM_POLYNOMIAL           (0x1021U)
#define CRC32_ISO_HDLC_POLYNOMIAL         (0xEDB88320UL)
//******************************** Defines **********************************//

void crc8_smbus_init(crc8_smbus_context_t *context)
{
    if (context == NULL) {
        return;
    }

    context->value = 0U;
}

void crc8_smbus_update(crc8_smbus_context_t *context, const uint8_t *data, uint32_t length)
{
    uint8_t bitIndex;
    uint32_t dataIndex;

    if ((context == NULL) || ((data == NULL) && (length != 0U))) {
        return;
    }

    for (dataIndex = 0U; dataIndex < length; dataIndex++) {
        context->value ^= data[dataIndex];
        for (bitIndex = 0U; bitIndex < 8U; bitIndex++) {
            if ((context->value & 0x80U) != 0U) {
                context->value = (uint8_t)((context->value << 1U) ^ CRC8_SMBUS_POLYNOMIAL);
            } else {
                context->value <<= 1U;
            }
        }
    }
}

uint8_t crc8_smbus_finalize(const crc8_smbus_context_t *context)
{
    return (context == NULL) ? 0U : context->value;
}

uint8_t crc8_smbus_calculate(const uint8_t *data, uint32_t length)
{
    crc8_smbus_context_t context;

    crc8_smbus_init(&context);
    crc8_smbus_update(&context, data, length);
    return crc8_smbus_finalize(&context);
}

void crc16_xmodem_init(crc16_xmodem_context_t *context)
{
    if (context == NULL) {
        return;
    }

    context->value = 0U;
}

void crc16_xmodem_update(crc16_xmodem_context_t *context, const uint8_t *data, uint32_t length)
{
    uint8_t bitIndex;
    uint32_t dataIndex;

    if ((context == NULL) || ((data == NULL) && (length != 0U))) {
        return;
    }

    for (dataIndex = 0U; dataIndex < length; dataIndex++) {
        context->value ^= (uint16_t)((uint16_t)data[dataIndex] << 8U);
        for (bitIndex = 0U; bitIndex < 8U; bitIndex++) {
            if ((context->value & 0x8000U) != 0U) {
                context->value = (uint16_t)((context->value << 1U) ^ CRC16_XMODEM_POLYNOMIAL);
            } else {
                context->value <<= 1U;
            }
        }
    }
}

uint16_t crc16_xmodem_finalize(const crc16_xmodem_context_t *context)
{
    return (context == NULL) ? 0U : context->value;
}

uint16_t crc16_xmodem_calculate(const uint8_t *data, uint32_t length)
{
    crc16_xmodem_context_t context;

    crc16_xmodem_init(&context);
    crc16_xmodem_update(&context, data, length);
    return crc16_xmodem_finalize(&context);
}

void crc32_iso_hdlc_init(crc32_iso_hdlc_context_t *context)
{
    if (context == NULL) {
        return;
    }

    context->value = 0xFFFFFFFFUL;
}

void crc32_iso_hdlc_update(crc32_iso_hdlc_context_t *context, const uint8_t *data, uint32_t length)
{
    uint8_t bitIndex;
    uint32_t dataIndex;

    if ((context == NULL) || ((data == NULL) && (length != 0U))) {
        return;
    }

    for (dataIndex = 0U; dataIndex < length; dataIndex++) {
        context->value ^= (uint32_t)data[dataIndex];
        for (bitIndex = 0U; bitIndex < 8U; bitIndex++) {
            if ((context->value & 1UL) != 0UL) {
                context->value = (context->value >> 1U) ^ CRC32_ISO_HDLC_POLYNOMIAL;
            } else {
                context->value >>= 1U;
            }
        }
    }
}

uint32_t crc32_iso_hdlc_finalize(const crc32_iso_hdlc_context_t *context)
{
    return (context == NULL) ? 0U : (context->value ^ 0xFFFFFFFFUL);
}

uint32_t crc32_iso_hdlc_calculate(const uint8_t *data, uint32_t length)
{
    crc32_iso_hdlc_context_t context;

    crc32_iso_hdlc_init(&context);
    crc32_iso_hdlc_update(&context, data, length);
    return crc32_iso_hdlc_finalize(&context);
}
