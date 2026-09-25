/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file ymodem_transport.h
 * @brief YMODEM Receiver 传输层发送合同
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef YMODEM_TRANSPORT_H
#define YMODEM_TRANSPORT_H

#include "platform_error.h"

typedef struct
{
    platform_error_t (*write)(
        void *context,
        const uint8_t *data,
        uint32_t length,
        uint32_t timeoutMs);
    void *context;
} ymodem_transport_t;

#endif
