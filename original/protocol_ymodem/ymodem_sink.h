/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file ymodem_sink.h
 * @brief YMODEM 文件接收 Sink 合同
 * @author YaoQian Wang
 * @date 2026-09-14
 * @version V1.0
 *
 *****************************************************************************/

#ifndef YMODEM_SINK_H
#define YMODEM_SINK_H

#include "platform_error.h"

typedef struct
{
    platform_error_t (*begin)(void *context, const char *filename, uint32_t fileSize);
    platform_error_t (*write)(void *context, const uint8_t *data, uint32_t length);
    platform_error_t (*end)(void *context);
    void (*abort)(void *context);
    void *context;
} ymodem_sink_t;

#endif
