/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file ymodem_parser.h
 * @brief YMODEM 增量 Packet Parser 公共接口
 * @author YaoQian Wang
 * @date 2026-09-14
 * @version V1.0
 *
 *****************************************************************************/

#ifndef YMODEM_PARSER_H
#define YMODEM_PARSER_H

//******************************** Includes *********************************//
#include "ymodem_def.h"
//******************************** Includes *********************************//

//******************************** Types ***********************************//
/** @brief YMODEM Parser 的增量解析状态。 */
typedef enum
{
    YMODEM_PARSER_STATE_WAIT_START = 0, /**< 等待 SOH、STX、EOT 或 CAN。 */
    YMODEM_PARSER_STATE_COLLECT_PACKET /**< 正在收集当前 Packet 的剩余字节。 */
} ymodem_parser_state_t;

/** @brief YMODEM Parser 跨 UART read 调用保存的运行上下文。 */
typedef struct
{
    ymodem_parser_state_t state; /**< 当前增量解析状态。 */
    uint16_t expectedLength;      /**< 当前 Packet 完整帧长度。 */
    uint16_t receivedLength;      /**< 当前 Packet 已缓存长度。 */
    uint8_t packetBuffer[YMODEM_PACKET_MAX_SIZE]; /**< 当前 Packet 原始帧 Buffer。 */
} ymodem_parser_t;
//******************************** Types ***********************************//

//******************************** Declaring *******************************//
/**
 * @brief 初始化 YMODEM 增量 Parser
 * @param[out] parser : Parser 对象；不得为空
 */
void ymodem_parser_init(ymodem_parser_t *parser);
/**
 * @brief 丢弃当前半包并回到等待包起始状态
 * @param[in,out] parser : Parser 对象；为空时不执行操作
 */
void ymodem_parser_reset(ymodem_parser_t *parser);
/**
 * @brief 向 Parser 追加一个 UART 字节
 * @param[in,out] parser : Parser 对象；不得为空
 * @param[in] byte : 新收到的 UART 字节
 * @param[out] event : 本次调用产生的事件；不得为空
 * @param[out] packet : 完整 Packet 输出；不得为空
 * @return PLATFORM_ERR_OK 表示字节已处理；其他值表示接口参数或内部状态错误。
 * @note Packet 的 data 指针指向 Parser 内部 Buffer，在下一次 Packet 解析前有效。
 */
platform_error_t ymodem_parser_feed_byte(
    ymodem_parser_t *parser,
    uint8_t byte,
    ymodem_parser_event_t *event,
    ymodem_packet_t *packet);
//******************************** Declaring *******************************//

#endif
