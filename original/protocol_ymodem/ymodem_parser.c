/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file ymodem_parser.c
 * @brief YMODEM 增量 Packet Parser 实现
 * @author YaoQian Wang
 * @date 2026-09-14
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "crc.h"
#include "ymodem_parser.h"
//******************************** Includes *********************************//

//******************************** Private Functions ************************//
static uint16_t ymodem_parser_get_data_size(uint8_t control)
{
    return (control == YMODEM_SOH) ?
           YMODEM_PACKET_DATA_SIZE_128 : YMODEM_PACKET_DATA_SIZE_1K;
}

/** @brief 将当前完整但校验失败的候选帧转换为 Parser 错误事件。 */
static void ymodem_parser_emit_packet_error(
    ymodem_parser_t *parser,
    ymodem_parser_event_t *event,
    ymodem_packet_t *packet)
{
    packet->blockNumber = 0U;
    packet->data = NULL;
    packet->dataSize = 0U;
    *event = YMODEM_PARSER_EVENT_PACKET_ERROR;
    ymodem_parser_reset(parser);
}

/** @brief 校验完整 Packet 并生成 Parser 输出事件。 */
static void ymodem_parser_complete_packet(
    ymodem_parser_t *parser,
    ymodem_parser_event_t *event,
    ymodem_packet_t *packet)
{
    uint16_t dataSize;
    uint16_t receivedCrc;
    uint16_t calculatedCrc;

    dataSize = ymodem_parser_get_data_size(parser->packetBuffer[0]);
    if ((uint8_t)(parser->packetBuffer[1] + parser->packetBuffer[2]) != 0xFFU) {
        ymodem_parser_emit_packet_error(parser, event, packet);
        return;
    }

    receivedCrc = (uint16_t)(((uint16_t)parser->packetBuffer[3U + dataSize] << 8U) |
                             parser->packetBuffer[4U + dataSize]);
    calculatedCrc = crc16_xmodem_calculate(&parser->packetBuffer[3], dataSize);
    if (receivedCrc != calculatedCrc) {
        ymodem_parser_emit_packet_error(parser, event, packet);
        return;
    }

    packet->blockNumber = parser->packetBuffer[1];
    packet->data = &parser->packetBuffer[3];
    packet->dataSize = dataSize;
    *event = YMODEM_PARSER_EVENT_PACKET;
    ymodem_parser_reset(parser);
}
//******************************** Private Functions ************************//

//******************************** Public Functions *************************//
void ymodem_parser_init(ymodem_parser_t *parser)
{
    ymodem_parser_reset(parser);
}

void ymodem_parser_reset(ymodem_parser_t *parser)
{
    if (parser == NULL) {
        return;
    }

    parser->state = YMODEM_PARSER_STATE_WAIT_START;
    parser->expectedLength = 0U;
    parser->receivedLength = 0U;
}

platform_error_t ymodem_parser_feed_byte(
    ymodem_parser_t *parser,
    uint8_t byte,
    ymodem_parser_event_t *event,
    ymodem_packet_t *packet)
{
    uint16_t dataSize;

    if ((parser == NULL) || (event == NULL) || (packet == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    *event = YMODEM_PARSER_EVENT_NONE;
    packet->blockNumber = 0U;
    packet->data = NULL;
    packet->dataSize = 0U;

    if (parser->state == YMODEM_PARSER_STATE_WAIT_START) {
        if ((byte == YMODEM_SOH) || (byte == YMODEM_STX)) {
            dataSize = ymodem_parser_get_data_size(byte);
            parser->packetBuffer[0] = byte;
            parser->expectedLength = (uint16_t)(dataSize + 5U);
            parser->receivedLength = 1U;
            parser->state = YMODEM_PARSER_STATE_COLLECT_PACKET;
        } else if (byte == YMODEM_EOT) {
            *event = YMODEM_PARSER_EVENT_EOT;
        } else if (byte == YMODEM_CAN) {
            *event = YMODEM_PARSER_EVENT_CAN;
        }

        return PLATFORM_ERR_OK;
    }

    if (parser->state != YMODEM_PARSER_STATE_COLLECT_PACKET) {
        ymodem_parser_reset(parser);
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (parser->receivedLength >= YMODEM_PACKET_MAX_SIZE) {
        ymodem_parser_reset(parser);
        return PLATFORM_ERR_OVERFLOW;
    }

    parser->packetBuffer[parser->receivedLength] = byte;
    parser->receivedLength++;
    if (parser->receivedLength == parser->expectedLength) {
        ymodem_parser_complete_packet(parser, event, packet);
    }

    return PLATFORM_ERR_OK;
}
//******************************** Public Functions *************************//
