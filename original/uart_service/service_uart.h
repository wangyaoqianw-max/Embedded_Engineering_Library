/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file service_uart.h
 * @brief UART 传输 Service 公共接口
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *
 *****************************************************************************/

#ifndef SERVICE_UART_H
#define SERVICE_UART_H

//******************************** Includes *********************************//
#include "ring_buffer.h"
#include "platform_os.h"
#include "platform_uart.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define SERVICE_UART_INITIALIZER             {0}
#define SERVICE_UART_EVENT_RX_AVAILABLE      (1U << 0)
#define SERVICE_UART_EVENT_DATA_LOSS         (1U << 1)
#define SERVICE_UART_EVENT_ERROR             (1U << 2)
#define SERVICE_UART_EVENT_STOPPED           (1U << 3)
//******************************** Defines *********************************//

//******************************** Types ***********************************//
/**
 * @brief UART Service 生命周期状态
 * @note 状态由 Service 管理；RUNNING 状态下 UART RX callback 是 RingBuffer 唯一 Producer。
 */
typedef enum
{
    SERVICE_UART_STATE_UNINITIALIZED = 0,
    SERVICE_UART_STATE_INITIALIZED,
    SERVICE_UART_STATE_RUNNING,
    SERVICE_UART_STATE_STOPPING,
    SERVICE_UART_STATE_STOPPED,
    SERVICE_UART_STATE_ERROR,
    SERVICE_UART_STATE_MAX
} service_uart_state_t;

/**
 * @brief UART Service 单笔 TX transaction 运行状态
 */
typedef enum
{
    SERVICE_UART_TX_STATE_IDLE = 0,
    SERVICE_UART_TX_STATE_ACTIVE,
    SERVICE_UART_TX_STATE_MAX
} service_uart_tx_state_t;

/**
 * @brief UART Service 初始化配置
 * @note 全部指针指向的对象及存储均由 APP / Caller 持有，并必须至少有效至 deinit 完成。
 */
typedef struct
{
    /** Platform UART 抽象对象；Service 不拥有其硬件生命周期。 */
    platform_uart_t *uart;
    /** UART DMA 持续接收使用的调用者存储。 */
    uint8_t *dmaRxBuffer;
    /** DMA 接收存储的字节数，必须大于 0。 */
    platform_size_t dmaRxBufferSize;
    /** SPSC RingBuffer 使用的调用者后备存储。 */
    uint8_t *ringBufferStorage;
    /** RingBuffer 后备存储长度，必须至少为 2。 */
    platform_size_t ringBufferStorageSize;
    /** UART Service 所属唯一 Task execution context 的 Platform 线程对象。 */
    platform_thread_t *ownerThread;
} service_uart_config_t;

/**
 * @brief UART Service 当前 RX Session 的运行上下文
 * @note state、lastError 与 dataLossOccurred 可由 UART callback 和 Consumer Task 并发观察。
 */
typedef struct
{
    /** 当前 Service 生命周期状态。 */
    volatile service_uart_state_t state;
    /** RX callback 为 Producer、Consumer Task 为 Consumer 的 SPSC RingBuffer。 */
    ring_buffer_t rxRingBuffer;
    /** 当前 Session 的 UART 或 Service runtime error；RingBuffer overflow 不写入此字段。 */
    volatile platform_error_t lastError;
    /** 当前 Session 的 sticky 数据丢失标记；新 Session 成功启动时清除。 */
    volatile platform_bool_t dataLossOccurred;
    /** 当前单笔 TX transaction 的运行状态。 */
    volatile service_uart_tx_state_t txState;
    /** 最近一次 TX transaction 的终止结果。 */
    volatile platform_error_t txResult;
} service_uart_context_t;

/**
 * @brief UART Service 跨 RX Session 累计统计
 * @note 字段为 best-effort 快照；不通过锁或 IRQ masking 强制获得事务一致性。
 */
typedef struct
{
    /** 收到的 RX_DATA 事件数量。 */
    volatile uint32_t rxEventCount;
    /** RX_DATA 输入字节累计值。 */
    volatile uint32_t rxBytesReceived;
    /** 已成功复制到 RingBuffer 的字节累计值。 */
    volatile uint32_t rxBytesBuffered;
    /** 已被 Consumer Task 读取的字节累计值。 */
    volatile uint32_t rxBytesRead;
    /** 因 RingBuffer 无法容纳而丢弃的输入字节累计值。 */
    volatile uint32_t rxBytesDropped;
    /** 发生 Partial Write 或 Full Drop 的事件累计值。 */
    volatile uint32_t ringBufferOverflowCount;
    /** RingBuffer 历史最大可读字节数，不因读取或新 Session 而下降。 */
    volatile platform_size_t ringBufferHighWaterMark;
    /** 收到的 Platform UART ERROR 事件累计值。 */
    volatile uint32_t uartErrorCount;
    /** 收到的 Platform UART CANCELED 事件累计值。 */
    volatile uint32_t cancelCount;
    /** TX 请求次数。 */
    volatile uint32_t txRequestCount;
    /** TX 正常完成次数。 */
    volatile uint32_t txCompleteCount;
    /** TX 正常完成字节总数。 */
    volatile uint32_t txBytesCompleted;
    /** 因已有活动 TX transaction 被拒绝的次数。 */
    volatile uint32_t txBusyRejectCount;
    /** TX 超时次数。 */
    volatile uint32_t txTimeoutCount;
    /** TX 错误次数。 */
    volatile uint32_t txErrorCount;
    /** TX 取消次数。 */
    volatile uint32_t txCancelCount;
} service_uart_statistics_t;

/**
 * @brief UART Service 当前运行状态快照
 */
typedef struct
{
    /** 当前 Service 生命周期状态。 */
    service_uart_state_t state;
    /** 当前 Session 的最近 runtime error。 */
    platform_error_t lastError;
    /** 当前 Session 是否发生过数据丢失。 */
    platform_bool_t dataLossOccurred;
} service_uart_status_t;

/**
 * @brief UART Service 对象
 * @note 对象和全部 Config 指向的资源均由 APP / Caller 持有；使用前必须零初始化。
 */
typedef struct
{
    /** 不随运行时变化的依赖与存储配置副本。 */
    service_uart_config_t config;
    /** 当前 RX Session 的运行上下文。 */
    service_uart_context_t context;
    /** 跨 Session 累计统计。 */
    service_uart_statistics_t statistics;
} service_uart_t;
//******************************** Types ***********************************//

//******************************** Declaring *******************************//
/**
 * @brief 初始化 UART Service 并绑定 Platform UART 异步回调
 * @param[in,out] service : 使用 SERVICE_UART_INITIALIZER 初始化的 Service 对象
 * @param[in] config      : 调用者持有的 UART、DMA、RingBuffer 与 Consumer Task 配置
 * @return PLATFORM_ERR_OK 成功；其他值表示参数、状态或回调绑定失败
 * @note 本函数不初始化或启动 UART 硬件，不接管任何外部存储的所有权。
 */
platform_error_t service_uart_init(service_uart_t *service,
                                   const service_uart_config_t *config);
/**
 * @brief 开启一个新的 UART RX Session
 * @param[in,out] service : 已初始化的 UART Service 对象
 * @return PLATFORM_ERR_OK 成功；其他值表示状态或 Platform UART 异步接收启动失败
 * @note 成功启动会丢弃 RingBuffer 中旧 Session 的未读数据，但不清除累计统计。
 */
platform_error_t service_uart_start(service_uart_t *service);
/**
 * @brief 取消当前 UART RX Session 并停止 Service 接收
 * @param[in,out] service : 正在运行的 UART Service 对象
 * @return PLATFORM_ERR_OK 成功；其他值表示状态、Platform UART 取消或 Task 通知失败
 * @note 仅 Service 可以取消其拥有的活动 RX Session。
 * @note 若在取消完成后的通知阶段失败，Service 已处于 STOPPED；调用者必须通过
 * service_uart_get_status() 获取真实状态，不能仅凭本函数错误返回推断仍在运行。
 */
platform_error_t service_uart_stop(service_uart_t *service);
/**
 * @brief 同步发送一笔 UART 数据并等待 DMA transaction 终止
 * @param[in,out] service    : 正在运行且由 ownerThread 调用的 Service 对象
 * @param[in] data           : 调用者持有的发送缓冲区
 * @param[in] dataLength     : 发送字节数
 * @param[in] timeoutMs      : 总超时时间，单位为毫秒
 * @return PLATFORM_ERR_OK 成功；其他值表示状态、发送、取消、超时或传输错误
 * @warning 函数返回前 DMA 不再访问 data 指向的调用者缓冲区。
 */
platform_error_t service_uart_write(service_uart_t *service,
                                    const uint8_t *data,
                                    platform_size_t dataLength,
                                    uint32_t timeoutMs);
/**
 * @brief 解绑 Platform UART 回调并清理 UART Service 对象
 * @param[in,out] service : 已初始化且不存在活动 RX Session 的 Service 对象
 * @return PLATFORM_ERR_OK 成功；其他值表示状态或回调解绑失败
 * @note 不停止 UART 硬件、不销毁 Consumer Task，也不释放或清空调用者持有的存储。
 */
platform_error_t service_uart_deinit(service_uart_t *service);
/**
 * @brief 非阻塞读取当前 RingBuffer 中已缓存的 RX 数据
 * @param[in,out] service    : 已初始化的 UART Service 对象
 * @param[out] buffer        : 调用者提供的输出缓冲区
 * @param[in] bufferSize     : 输出缓冲区容量
 * @param[out] readLength    : 实际读取的字节数
 * @return PLATFORM_ERR_OK 成功；其他值表示状态、参数或 RingBuffer 读取结果
 * @note 仅专用 Consumer Task 可以调用本接口。
 */
platform_error_t service_uart_read(service_uart_t *service,
                                   uint8_t *buffer,
                                   platform_size_t bufferSize,
                                   platform_size_t *readLength);
/**
 * @brief 等待 UART Service 的事件唤醒
 * @param[in,out] service   : 已初始化的 UART Service 对象
 * @param[in] timeoutMs     : 等待超时时间，单位为毫秒
 * @param[out] events       : 返回收到的 Service 事件位
 * @return PLATFORM_ERR_OK 成功；其他值表示状态、参数、超时或 Platform Notify 失败
 * @note 通知仅是唤醒提示，数据可读性和运行状态仍以 Service 查询接口为准。
 */
platform_error_t service_uart_wait_event(service_uart_t *service,
                                         uint32_t timeoutMs,
                                         uint32_t *events);
/**
 * @brief 查询当前 RingBuffer 中的可读字节数
 * @param[in] service          : 已初始化的 UART Service 对象
 * @param[out] readableSize    : 当前可读取的字节数
 * @return PLATFORM_ERR_OK 成功；其他值表示状态、参数或 RingBuffer 查询失败
 */
platform_error_t service_uart_get_readable_size(const service_uart_t *service,
                                                platform_size_t *readableSize);
/**
 * @brief 获取 UART Service 当前运行状态快照
 * @param[in] service : 已初始化的 UART Service 对象
 * @param[out] status : 输出状态快照
 * @return PLATFORM_ERR_OK 成功；其他值表示状态或参数错误
 */
platform_error_t service_uart_get_status(const service_uart_t *service,
                                         service_uart_status_t *status);
/**
 * @brief 获取 UART Service 累计统计快照
 * @param[in] service       : 已初始化的 UART Service 对象
 * @param[out] statistics   : 输出统计快照
 * @return PLATFORM_ERR_OK 成功；其他值表示状态或参数错误
 * @note 统计为无锁 best-effort 快照，不保证多个字段具有同一时刻的一致性。
 */
platform_error_t service_uart_get_statistics(
    const service_uart_t *service,
    service_uart_statistics_t *statistics);
/**
 * @brief 清除 UART Service 的累计统计
 * @param[in,out] service : 已初始化且 Consumer 静止的 UART Service 对象
 * @return PLATFORM_ERR_OK 成功；其他值表示状态错误
 * @note 本函数不清除当前 Session 的 dataLossOccurred 或 lastError。
 */
platform_error_t service_uart_clear_statistics(service_uart_t *service);
//******************************** Declaring *******************************//

#endif
