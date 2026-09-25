/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file ymodem_def.h
 * @brief YMODEM 协议固定定义
 * @author YaoQian Wang
 * @date 2026-09-14
 * @version V1.0
 *
 *****************************************************************************/

#ifndef YMODEM_DEF_H
#define YMODEM_DEF_H

#include "platform_error.h"

#define YMODEM_SOH                            (0x01U)
#define YMODEM_STX                            (0x02U)
#define YMODEM_EOT                            (0x04U)
#define YMODEM_ACK                            (0x06U)
#define YMODEM_NAK                            (0x15U)
#define YMODEM_CAN                            (0x18U)
#define YMODEM_C                              (0x43U)

#define YMODEM_PACKET_DATA_SIZE_128           (128U)
#define YMODEM_PACKET_DATA_SIZE_1K            (1024U)
#define YMODEM_PACKET_MAX_SIZE                (1029U)

typedef enum
{
    YMODEM_PARSER_EVENT_NONE = 0,
    YMODEM_PARSER_EVENT_PACKET,
    YMODEM_PARSER_EVENT_EOT,
    YMODEM_PARSER_EVENT_CAN,
    YMODEM_PARSER_EVENT_PACKET_ERROR
} ymodem_parser_event_t;

typedef struct
{
    uint8_t blockNumber;
    const uint8_t *data;
    uint16_t dataSize;
} ymodem_packet_t;

#endif
