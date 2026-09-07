/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file ring_buffer.h
 * @brief SPSC 字节 RingBuffer 公共接口
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 * @note 后备存储由调用者持有，实际可用容量为 storageSize - 1。
 * @warning 仅支持单 Producer / 单 Consumer；reset 仅允许在双方静止时调用。
 *****************************************************************************/

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

//******************************** Includes *********************************//
#include "platform_error.h"
//******************************** Includes *********************************//

//******************************** Types ************************************//
/**
 * @brief SPSC RingBuffer 对象
 * @note Producer 仅写 writeIndex，Consumer 仅写 readIndex。
 */
typedef struct
{
    uint8_t *storage;
    platform_size_t storageSize;
    volatile platform_size_t readIndex;
    volatile platform_size_t writeIndex;
} ring_buffer_t;
//******************************** Types ************************************//

//******************************** Declaring *********************************//
/**
 * @brief 初始化 RingBuffer
 * @param[in,out] ringBuffer RingBuffer 对象。
 * @param[in] storage 调用者持有的后备存储。
 * @param[in] storageSize 后备存储实际长度，最小为 2。
 * @return PLATFORM_ERR_OK 成功；其他值表示参数错误。
 */
platform_error_t ring_buffer_init(ring_buffer_t *ringBuffer,
                                  uint8_t *storage,
                                  platform_size_t storageSize);

/**
 * @brief 重置读写索引
 * @param[in,out] ringBuffer 已初始化的 RingBuffer 对象。
 * @return PLATFORM_ERR_OK 成功；其他值表示对象无效。
 * @warning 不清零后备存储，且只允许 Producer 与 Consumer 均静止时调用。
 */
platform_error_t ring_buffer_reset(ring_buffer_t *ringBuffer);

/**
 * @brief 向 RingBuffer 写入字节流
 * @param[in,out] ringBuffer 已初始化的 RingBuffer 对象。
 * @param[in] data 输入字节；dataLength 为 0 时允许为 NULL。
 * @param[in] dataLength 请求写入长度。
 * @param[out] writtenLength 实际写入长度，不能为空。
 * @return 请求未完整写入时返回 PLATFORM_ERR_OVERFLOW。
 * @note 使用 Partial Write，不覆盖尚未读取的旧数据。
 */
platform_error_t ring_buffer_write(ring_buffer_t *ringBuffer,
                                   const uint8_t *data,
                                   platform_size_t dataLength,
                                   platform_size_t *writtenLength);

/**
 * @brief 从 RingBuffer 读取当前可用字节
 * @param[in,out] ringBuffer 已初始化的 RingBuffer 对象。
 * @param[out] buffer 输出缓冲区；bufferSize 为 0 时允许为 NULL。
 * @param[in] bufferSize 最大读取长度。
 * @param[out] readLength 实际读取长度，不能为空。
 * @return 非零长度读取时为空返回 PLATFORM_ERR_EMPTY。
 */
platform_error_t ring_buffer_read(ring_buffer_t *ringBuffer,
                                  uint8_t *buffer,
                                  platform_size_t bufferSize,
                                  platform_size_t *readLength);

/**
 * @brief 查询当前可读取字节数
 * @param[in] ringBuffer 已初始化的 RingBuffer 对象。
 * @param[out] readableSize 当前可读取字节数，不能为空。
 * @return PLATFORM_ERR_OK 成功；其他值表示参数或初始化状态错误。
 */
platform_error_t ring_buffer_get_readable_size(const ring_buffer_t *ringBuffer,
                                               platform_size_t *readableSize);

/**
 * @brief 查询当前可写入字节数
 * @param[in] ringBuffer 已初始化的 RingBuffer 对象。
 * @param[out] freeSize 当前可写入字节数，不能为空。
 * @return PLATFORM_ERR_OK 成功；其他值表示参数或初始化状态错误。
 */
platform_error_t ring_buffer_get_free_size(const ring_buffer_t *ringBuffer,
                                           platform_size_t *freeSize);
//******************************** Declaring *********************************//

#endif
