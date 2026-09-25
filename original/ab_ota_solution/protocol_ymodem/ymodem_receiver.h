/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file ymodem_receiver.h
 * @brief YMODEM Receiver 状态机公共接口
 * @author YaoQian Wang
 * @date 2026-09-14
 * @version V1.0
 *
 *****************************************************************************/

#ifndef YMODEM_RECEIVER_H
#define YMODEM_RECEIVER_H

//******************************** Includes *********************************//
#include "ymodem_transport.h"
#include "ymodem_config.h"
#include "ymodem_parser.h"
#include "ymodem_sink.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define YMODEM_RECEIVER_INITIALIZER          {0}
//******************************** Defines **********************************//

//******************************** Types ***********************************//
/** @brief YMODEM Receiver 的 Session 状态。 */
typedef enum
{
    YMODEM_RECEIVER_STATE_UNINITIALIZED = 0, /**< 对象尚未初始化。 */
    YMODEM_RECEIVER_STATE_IDLE,              /**< 已初始化，等待启动 Session。 */
    YMODEM_RECEIVER_STATE_WAIT_HEADER,       /**< 已发送 C，等待非空 Block 0。 */
    YMODEM_RECEIVER_STATE_RECEIVE_DATA,      /**< 已开始文件，等待数据 Packet。 */
    YMODEM_RECEIVER_STATE_WAIT_EOT_CONFIRM,  /**< 已收到第一个 EOT，等待第二个 EOT。 */
    YMODEM_RECEIVER_STATE_WAIT_END_HEADER,   /**< 等待结束用的空 Block 0。 */
    YMODEM_RECEIVER_STATE_FINISHED,          /**< 文件提交完成且 ACK 已发送。 */
    YMODEM_RECEIVER_STATE_ABORTED,           /**< Session 被远端或本地主动取消。 */
    YMODEM_RECEIVER_STATE_ERROR              /**< Session 因不可恢复错误结束。 */
} ymodem_receiver_state_t;

/** @brief Receiver 初始化时绑定的传输接口和文件 Sink。 */
typedef struct
{
    ymodem_transport_t transport; /**< 控制字节发送所使用的传输接口。 */
    ymodem_sink_t sink;   /**< 文件生命周期和数据写入回调。 */
} ymodem_receiver_config_t;

/** @brief YMODEM Block 0 中已解析的文件元数据。 */
typedef struct
{
    char filename[YMODEM_CFG_FILENAME_MAX_LEN + 1U]; /**< Block 0 文件名，以 NUL 结尾。 */
    uint32_t fileSize;                              /**< Block 0 声明的文件总长度。 */
    uint32_t modificationTime; /**< ASCII 八进制修改时间转换后的秒数。 */
    uint32_t fileMode;         /**< ASCII 八进制文件权限转换后的数值。 */
    uint32_t serialNumber;     /**< ASCII 八进制发送方序号，未提供时为 0。 */
} ymodem_receiver_block0_metadata_t;

/** @brief Receiver 当前 Session 的运行上下文。 */
typedef struct
{
    ymodem_receiver_state_t state; /**< 当前 Session 状态机状态。 */
    ymodem_parser_t parser;        /**< 增量字节 Parser 上下文。 */
    uint8_t expectedBlock;          /**< 下一个期望接收的 Packet 序号。 */
    uint32_t retryCount;            /**< 当前等待目标的连续重试次数。 */
    ymodem_receiver_block0_metadata_t block0Metadata; /**< Block 0 已解析的完整元数据。 */
    uint32_t receivedSize;          /**< 已交给 Sink 的有效文件字节数。 */
    uint32_t lastActivityMs;        /**< 最近一次传输活动时间戳。 */
    platform_bool_t fileStarted;   /**< Sink begin 已成功且尚未 end/abort。 */
    platform_error_t lastError;    /**< 最近一次不可恢复错误。 */
} ymodem_receiver_context_t;

/** @brief Receiver 当前 Session 的诊断统计。 */
typedef struct
{
    uint32_t packetReceivedCount; /**< Parser 输出完整 Packet 的数量。 */
    uint32_t packetAcceptedCount; /**< 通过序号和 Sink 处理的 Packet 数量。 */
    uint32_t bytesReceived;       /**< 已接收的有效文件字节数。 */
    uint32_t bytesWritten;        /**< 已成功交给 Sink 的字节数。 */
    uint32_t crcErrorCount;       /**< Packet 校验失败次数。 */
    uint32_t sequenceErrorCount;  /**< Block 序号错误次数。 */
    uint32_t duplicatePacketCount; /**< 重复 Packet 次数。 */
    uint32_t timeoutCount;        /**< 超时事件次数。 */
    uint32_t retryCount;          /**< 实际发出的重试控制次数。 */
    uint32_t cancelCount;         /**< 远端或本地取消次数。 */
} ymodem_receiver_statistics_t;

/** @brief Receiver 对外提供的状态快照。 */
typedef struct
{
    ymodem_receiver_state_t state; /**< 当前 Session 状态。 */
    platform_error_t lastError;    /**< 最近一次错误。 */
    ymodem_receiver_block0_metadata_t block0Metadata; /**< 当前 Block 0 完整元数据。 */
    uint32_t receivedSize;          /**< 当前已接收有效长度。 */
} ymodem_receiver_status_t;

/** @brief YMODEM Receiver 对象，配置、上下文和统计分离保存。 */
typedef struct
{
    ymodem_receiver_config_t config; /**< 初始化后不变的依赖绑定。 */
    ymodem_receiver_context_t context; /**< 当前 Session 可变状态。 */
    ymodem_receiver_statistics_t statistics; /**< 当前对象累计统计。 */
} ymodem_receiver_t;
//******************************** Types ***********************************//

//******************************** Declaring *******************************//
/**
 * @brief 初始化 YMODEM Receiver 并绑定 Transport 与 Sink
 * @param[out] receiver : 使用 YMODEM_RECEIVER_INITIALIZER 初始化的对象
 * @param[in] config : 调用者持有的 Transport 与 Sink 配置
 * @return PLATFORM_ERR_OK 成功；其他值表示参数错误。
 */
platform_error_t ymodem_receiver_init(
    ymodem_receiver_t *receiver,
    const ymodem_receiver_config_t *config);
/**
 * @brief 启动一个新的 Single-file YMODEM Session
 * @param[in,out] receiver : 已初始化的 Receiver
 * @return PLATFORM_ERR_OK 成功；其他值表示状态或 Transport 发送错误。
 */
platform_error_t ymodem_receiver_start(ymodem_receiver_t *receiver);
/**
 * @brief 向 Receiver 追加一个传输层接收字节
 * @param[in,out] receiver : 正在运行的 Receiver
 * @param[in] byte : 新接收到的字节
 * @param[in] nowMs : 调用者提供的单调毫秒时间戳
 * @return PLATFORM_ERR_OK 表示字节已处理；其他值表示协议或 Sink 失败。
 */
platform_error_t ymodem_receiver_feed_byte(
    ymodem_receiver_t *receiver,
    uint8_t byte,
    uint32_t nowMs);
/**
 * @brief 处理当前时间下的超时重试
 * @param[in,out] receiver : 正在运行的 Receiver
 * @param[in] nowMs : 调用者提供的单调毫秒时间戳
 * @return PLATFORM_ERR_OK 表示未超时或已执行有界重试；超限返回错误。
 */
platform_error_t ymodem_receiver_tick(ymodem_receiver_t *receiver, uint32_t nowMs);
/**
 * @brief 主动取消当前 Session
 * @param[in,out] receiver : 正在运行或等待中的 Receiver
 * @return PLATFORM_ERR_OK 成功；其他值表示 Transport 取消序列发送失败。
 */
platform_error_t ymodem_receiver_cancel(ymodem_receiver_t *receiver);
/**
 * @brief 获取 Receiver 当前状态快照
 * @param[in] receiver : 已初始化的 Receiver
 * @param[out] status : 状态输出
 * @return PLATFORM_ERR_OK 成功；其他值表示参数错误。
 */
platform_error_t ymodem_receiver_get_status(
    const ymodem_receiver_t *receiver,
    ymodem_receiver_status_t *status);
/**
 * @brief 获取 Receiver 统计快照
 * @param[in] receiver : 已初始化的 Receiver
 * @param[out] statistics : 统计输出
 * @return PLATFORM_ERR_OK 成功；其他值表示参数错误。
 */
platform_error_t ymodem_receiver_get_statistics(
    const ymodem_receiver_t *receiver,
    ymodem_receiver_statistics_t *statistics);
//******************************** Declaring *******************************//

#endif
