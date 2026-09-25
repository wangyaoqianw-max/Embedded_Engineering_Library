/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file crc.h
 * @brief CRC Common 公共接口
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

#ifndef CRC_H
#define CRC_H

//******************************** Includes *********************************//
#include <stddef.h>
#include <stdint.h>
//******************************** Includes *********************************//

//******************************** Types ***********************************//
typedef struct
{
    /** 当前 CRC-8/SMBUS 流式计算内部值。 */
    uint8_t value;
} crc8_smbus_context_t;

typedef struct
{
    /** 当前 CRC-16/XMODEM 流式计算内部值。 */
    uint16_t value;
} crc16_xmodem_context_t;

typedef struct
{
    /** 当前 CRC-32/ISO-HDLC 流式计算内部值，尚未执行 final xor。 */
    uint32_t value;
} crc32_iso_hdlc_context_t;
//******************************** Types ***********************************//

//******************************** Declaring *******************************//
/**
 * @brief 初始化 CRC-8/SMBUS 流式计算上下文
 * @param[out] context : CRC 上下文；为空时不执行操作
 * @note 初始值固定为 0x00。
 */
void crc8_smbus_init(crc8_smbus_context_t *context);
/**
 * @brief 向 CRC-8/SMBUS 流式计算追加数据
 * @param[in,out] context : 已初始化的 CRC 上下文；为空时不执行操作
 * @param[in] data : 待计算数据；length 非零时不得为空
 * @param[in] length : 待计算字节数
 */
void crc8_smbus_update(crc8_smbus_context_t *context, const uint8_t *data, uint32_t length);
/**
 * @brief 获取 CRC-8/SMBUS 流式计算结果
 * @param[in] context : 已初始化的 CRC 上下文；为空时返回 0
 * @return 当前 CRC 结果；本接口不修改 context。
 */
uint8_t crc8_smbus_finalize(const crc8_smbus_context_t *context);
/**
 * @brief 一次性计算 CRC-8/SMBUS
 * @param[in] data : 待计算数据；length 非零时不得为空
 * @param[in] length : 待计算字节数
 * @return CRC-8/SMBUS 计算结果。
 */
uint8_t crc8_smbus_calculate(const uint8_t *data, uint32_t length);

/**
 * @brief 初始化 CRC-16/XMODEM 流式计算上下文
 * @param[out] context : CRC 上下文；为空时不执行操作
 * @note 初始值固定为 0x0000。
 */
void crc16_xmodem_init(crc16_xmodem_context_t *context);
/**
 * @brief 向 CRC-16/XMODEM 流式计算追加数据
 * @param[in,out] context : 已初始化的 CRC 上下文；为空时不执行操作
 * @param[in] data : 待计算数据；length 非零时不得为空
 * @param[in] length : 待计算字节数
 */
void crc16_xmodem_update(crc16_xmodem_context_t *context, const uint8_t *data, uint32_t length);
/**
 * @brief 获取 CRC-16/XMODEM 流式计算结果
 * @param[in] context : 已初始化的 CRC 上下文；为空时返回 0
 * @return 当前 CRC 结果；本接口不修改 context。
 */
uint16_t crc16_xmodem_finalize(const crc16_xmodem_context_t *context);
/**
 * @brief 一次性计算 CRC-16/XMODEM
 * @param[in] data : 待计算数据；length 非零时不得为空
 * @param[in] length : 待计算字节数
 * @return CRC-16/XMODEM 计算结果。
 */
uint16_t crc16_xmodem_calculate(const uint8_t *data, uint32_t length);

/**
 * @brief 初始化 CRC-32/ISO-HDLC 流式计算上下文
 * @param[out] context : CRC 上下文；为空时不执行操作
 * @note 初始值固定为 0xFFFFFFFF。
 */
void crc32_iso_hdlc_init(crc32_iso_hdlc_context_t *context);
/**
 * @brief 向 CRC-32/ISO-HDLC 流式计算追加数据
 * @param[in,out] context : 已初始化的 CRC 上下文；为空时不执行操作
 * @param[in] data : 待计算数据；length 非零时不得为空
 * @param[in] length : 待计算字节数
 */
void crc32_iso_hdlc_update(crc32_iso_hdlc_context_t *context, const uint8_t *data, uint32_t length);
/**
 * @brief 获取 CRC-32/ISO-HDLC 流式计算结果
 * @param[in] context : 已初始化的 CRC 上下文；为空时返回 0
 * @return 当前 CRC 结果；本接口不修改 context。
 */
uint32_t crc32_iso_hdlc_finalize(const crc32_iso_hdlc_context_t *context);
/**
 * @brief 一次性计算 CRC-32/ISO-HDLC
 * @param[in] data : 待计算数据；length 非零时不得为空
 * @param[in] length : 待计算字节数
 * @return CRC-32/ISO-HDLC 计算结果。
 */
uint32_t crc32_iso_hdlc_calculate(const uint8_t *data, uint32_t length);
//******************************** Declaring *******************************//

#endif
