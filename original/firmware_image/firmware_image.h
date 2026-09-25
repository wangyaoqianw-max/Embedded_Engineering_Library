/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file firmware_image.h
 * @brief Firmware Image Header V1 公共接口
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

#ifndef FIRMWARE_IMAGE_H
#define FIRMWARE_IMAGE_H

//******************************** Includes *********************************//
#include "firmware_def.h"
//******************************** Includes *********************************//

//******************************** Declaring *******************************//
/**
 * @brief 从固定 64 Byte little-endian Buffer 解码 Header
 * @param[in] rawHeader : 固定长度 Header Buffer；不得为空
 * @param[out] header : 解码后的 Header；不得为空
 * @return PLATFORM_ERR_OK 成功；其他值表示参数错误。
 * @note 本接口只做字段解码，不判定 Header 合法性。
 */
platform_error_t firmware_image_decode_header(
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE],
    firmware_image_header_t *header);
/**
 * @brief 编码 Firmware Image Header V1
 * @param[in] header : 源 Header；不得为空，Version 与 imageSize 必须有效
 * @param[out] rawHeader : 固定长度输出 Buffer；不得为空
 * @return PLATFORM_ERR_OK 成功；其他值表示参数或 V1 字段错误。
 * @note 本接口强制写入 V1 固定值和全零 reserved，并在最后写 Header CRC32。
 */
platform_error_t firmware_image_encode_header(
    const firmware_image_header_t *header,
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE]);
/**
 * @brief 按 Header V1 规则验证已读取的 Header
 * @param[in] rawHeader : 固定长度 Header Buffer；不得为空
 * @param[out] decodedHeader : 可选解码输出；为空时只返回验证结果
 * @return Header 的验证结果。
 * @note 验证顺序固定为 erased、magic、format、size、reserved、version、image size、CRC。
 */
firmware_image_validation_t firmware_image_validate_header(
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE],
    firmware_image_header_t *decodedHeader);
//******************************** Declaring *******************************//

#endif
