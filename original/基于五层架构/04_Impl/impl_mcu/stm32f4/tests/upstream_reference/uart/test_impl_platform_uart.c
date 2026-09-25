/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_impl_platform_uart.c
 * @brief USART1 DMA RX Phase 2A Host Tests
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.0
 *
 *****************************************************************************/

#include "platform_uart.h"
#include "usart.h"

UART_HandleTypeDef huart1;
HAL_StatusTypeDef g_fakeHalReceiveToIdleResult = HAL_OK;
HAL_StatusTypeDef g_fakeHalAbortReceiveResult = HAL_OK;
HAL_StatusTypeDef g_fakeHalAbortTransmitResult = HAL_OK;
HAL_StatusTypeDef g_fakeHalTransmitDmaResult = HAL_OK;
UART_HandleTypeDef *g_fakeHalReceiveToIdleUart;
uint8_t *g_fakeHalReceiveToIdleBuffer;
uint16_t g_fakeHalReceiveToIdleSize;
uint32_t g_fakeHalReceiveToIdleCallCount;
uint32_t g_fakeHalAbortReceiveCallCount;
uint32_t g_fakeHalAbortTransmitCallCount;
UART_HandleTypeDef *g_fakeHalTransmitDmaUart;
uint8_t *g_fakeHalTransmitDmaBuffer;
uint16_t g_fakeHalTransmitDmaSize;
uint32_t g_fakeHalTransmitDmaCallCount;
uint16_t g_fakeHalReceiveToIdleCallbackPosition;

#include "../../04_Impl/impl_mcu/impl_platform_uart.c"

#define TEST_ASSERT(condition)       \
    do {                             \
        if (!(condition)) {          \
            return __LINE__;         \
        }                            \
    } while (0)

typedef struct
{
    platform_uart_event_t events[8];
    uint32_t eventCount;
} event_record_t;

static void record_event(platform_uart_t *uart,
                         const platform_uart_event_t *event,
                         void *callbackContext)
{
    event_record_t *record = (event_record_t *)callbackContext;

    (void)uart;
    record->events[record->eventCount] = *event;
    record->eventCount++;
}

static platform_uart_config_t make_config(void)
{
    platform_uart_config_t config = {
        115200U,
        PLATFORM_UART_DATA_BITS_8,
        PLATFORM_UART_STOP_BITS_1,
        PLATFORM_UART_PARITY_NONE,
        PLATFORM_UART_FLOW_CONTROL_NONE,
        100U
    };

    return config;
}

static int test_blocking_config_mappings_remain_supported(void)
{
    UART_HandleTypeDef halUart = {0};
    platform_uart_config_t config = make_config();

    TEST_ASSERT(PLATFORM_ERR_OK == stm32_uart_apply_config(&halUart, &config));
    TEST_ASSERT(UART_WORDLENGTH_8B == halUart.Init.WordLength);
    TEST_ASSERT(UART_PARITY_NONE == halUart.Init.Parity);

    config.parity = PLATFORM_UART_PARITY_EVEN;
    TEST_ASSERT(PLATFORM_ERR_OK == stm32_uart_apply_config(&halUart, &config));
    TEST_ASSERT(UART_WORDLENGTH_9B == halUart.Init.WordLength);
    TEST_ASSERT(UART_PARITY_EVEN == halUart.Init.Parity);

    config.dataBits = PLATFORM_UART_DATA_BITS_9;
    TEST_ASSERT(PLATFORM_ERR_NOT_SUPPORTED == stm32_uart_apply_config(&halUart, &config));

    return 0;
}

static int make_started_uart(platform_uart_t *uart, event_record_t *record)
{
    platform_uart_config_t config = make_config();
    platform_error_t result = impl_platform_uart_usart1_construct(
        uart,
        "usart1",
        PLATFORM_DEVICE_CAP_NONE,
        &config,
        record_event,
        record);

    if (PLATFORM_ERR_OK != result) {
        return __LINE__;
    }

    result = uart->device.lifecycle->init(uart);
    if (PLATFORM_ERR_OK != result) {
        return __LINE__;
    }

    result = uart->device.lifecycle->start(uart);
    if (PLATFORM_ERR_OK != result) {
        return __LINE__;
    }

    return 0;
}

static void reset_fake_hal(void)
{
    g_fakeHalReceiveToIdleResult = HAL_OK;
    g_fakeHalAbortReceiveResult = HAL_OK;
    g_fakeHalAbortTransmitResult = HAL_OK;
    g_fakeHalTransmitDmaResult = HAL_OK;
    g_fakeHalReceiveToIdleUart = NULL;
    g_fakeHalReceiveToIdleBuffer = NULL;
    g_fakeHalReceiveToIdleSize = 0U;
    g_fakeHalReceiveToIdleCallCount = 0U;
    g_fakeHalAbortReceiveCallCount = 0U;
    g_fakeHalAbortTransmitCallCount = 0U;
    g_fakeHalTransmitDmaUart = NULL;
    g_fakeHalTransmitDmaBuffer = NULL;
    g_fakeHalTransmitDmaSize = 0U;
    g_fakeHalTransmitDmaCallCount = 0U;
    g_fakeHalReceiveToIdleCallbackPosition = 0U;
    huart1.ErrorCode = HAL_UART_ERROR_NONE;
}

static int test_write_async_starts_tx_and_rejects_second_transaction(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t firstData[] = {0x31U, 0x32U};
    uint8_t secondData[] = {0x33U};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_uart_write_async(&uart, firstData, sizeof(firstData)));
    TEST_ASSERT(1U == g_fakeHalTransmitDmaCallCount);
    TEST_ASSERT(&huart1 == g_fakeHalTransmitDmaUart);
    TEST_ASSERT(firstData == g_fakeHalTransmitDmaBuffer);
    TEST_ASSERT(sizeof(firstData) == g_fakeHalTransmitDmaSize);
    TEST_ASSERT(PLATFORM_ERR_BUSY ==
                platform_uart_write_async(&uart, secondData, sizeof(secondData)));

    return 0;
}

static int test_tx_complete_releases_transaction_before_notifying(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t firstData[] = {0x41U, 0x42U};
    uint8_t secondData[] = {0x43U};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_uart_write_async(&uart, firstData, sizeof(firstData)));

    HAL_UART_TxCpltCallback(&huart1);

    TEST_ASSERT(1U == record.eventCount);
    TEST_ASSERT(PLATFORM_UART_EVENT_TX_COMPLETE == record.events[0].type);
    TEST_ASSERT(PLATFORM_UART_DIRECTION_TX == record.events[0].direction);
    TEST_ASSERT(firstData == record.events[0].data);
    TEST_ASSERT(sizeof(firstData) == record.events[0].dataLength);
    TEST_ASSERT(PLATFORM_ERR_OK == record.events[0].error);
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_uart_write_async(&uart, secondData, sizeof(secondData)));
    TEST_ASSERT(2U == g_fakeHalTransmitDmaCallCount);

    return 0;
}

static int test_cancel_tx_releases_transaction_and_notifies(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t firstData[] = {0x51U};
    uint8_t secondData[] = {0x52U};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_uart_write_async(&uart, firstData, sizeof(firstData)));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_uart_cancel(&uart, PLATFORM_UART_DIRECTION_TX));
    TEST_ASSERT(1U == g_fakeHalAbortTransmitCallCount);
    TEST_ASSERT(1U == record.eventCount);
    TEST_ASSERT(PLATFORM_UART_EVENT_CANCELED == record.events[0].type);
    TEST_ASSERT(PLATFORM_UART_DIRECTION_TX == record.events[0].direction);
    TEST_ASSERT(PLATFORM_ERR_CANCELED == record.events[0].error);
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_uart_write_async(&uart, secondData, sizeof(secondData)));

    return 0;
}

static int test_cancel_both_terminates_independent_rx_and_tx_transactions(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t rxBuffer[16] = {0};
    uint8_t txData[] = {0x61U};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, rxBuffer, sizeof(rxBuffer)));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_write_async(&uart, txData, sizeof(txData)));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_uart_cancel(&uart, PLATFORM_UART_DIRECTION_BOTH));
    TEST_ASSERT(1U == g_fakeHalAbortReceiveCallCount);
    TEST_ASSERT(1U == g_fakeHalAbortTransmitCallCount);
    TEST_ASSERT(2U == record.eventCount);
    TEST_ASSERT(PLATFORM_UART_EVENT_CANCELED == record.events[0].type);
    TEST_ASSERT(PLATFORM_UART_DIRECTION_TX == record.events[0].direction);
    TEST_ASSERT(PLATFORM_UART_EVENT_CANCELED == record.events[1].type);
    TEST_ASSERT(PLATFORM_UART_DIRECTION_RX == record.events[1].direction);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, rxBuffer, sizeof(rxBuffer)));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_write_async(&uart, txData, sizeof(txData)));

    return 0;
}

static int test_lifecycle_stop_terminates_tx_without_business_cancel_event(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t txData[] = {0x71U};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_write_async(&uart, txData, sizeof(txData)));
    TEST_ASSERT(PLATFORM_ERR_OK == uart.device.lifecycle->stop(&uart));
    TEST_ASSERT(1U == g_fakeHalAbortTransmitCallCount);
    TEST_ASSERT(0U == record.eventCount);
    TEST_ASSERT(PLATFORM_ERR_OK == uart.device.lifecycle->start(&uart));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_write_async(&uart, txData, sizeof(txData)));

    return 0;
}

static int test_tx_dma_error_releases_tx_transaction(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t txData[] = {0x81U};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_write_async(&uart, txData, sizeof(txData)));

    huart1.ErrorCode = HAL_UART_ERROR_DMA;
    HAL_UART_ErrorCallback(&huart1);

    TEST_ASSERT(1U == record.eventCount);
    TEST_ASSERT(PLATFORM_UART_EVENT_ERROR == record.events[0].type);
    TEST_ASSERT(PLATFORM_UART_DIRECTION_TX == record.events[0].direction);
    TEST_ASSERT(PLATFORM_ERR_IO == record.events[0].error);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_write_async(&uart, txData, sizeof(txData)));

    return 0;
}

static int test_unattributed_dma_error_terminates_full_duplex_transactions(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t rxBuffer[16] = {0};
    uint8_t txData[] = {0x91U};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, rxBuffer, sizeof(rxBuffer)));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_write_async(&uart, txData, sizeof(txData)));

    huart1.ErrorCode = HAL_UART_ERROR_DMA;
    HAL_UART_ErrorCallback(&huart1);

    TEST_ASSERT(1U == record.eventCount);
    TEST_ASSERT(PLATFORM_UART_EVENT_ERROR == record.events[0].type);
    TEST_ASSERT(PLATFORM_UART_DIRECTION_BOTH == record.events[0].direction);
    TEST_ASSERT(PLATFORM_ERR_IO == record.events[0].error);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, rxBuffer, sizeof(rxBuffer)));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_write_async(&uart, txData, sizeof(txData)));

    return 0;
}

static int test_read_async_starts_dma_and_rejects_second_session(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t buffer[256] = {0};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, buffer, sizeof(buffer)));
    TEST_ASSERT(1U == g_fakeHalReceiveToIdleCallCount);
    TEST_ASSERT(&huart1 == g_fakeHalReceiveToIdleUart);
    TEST_ASSERT(buffer == g_fakeHalReceiveToIdleBuffer);
    TEST_ASSERT(sizeof(buffer) == g_fakeHalReceiveToIdleSize);
    TEST_ASSERT(PLATFORM_ERR_BUSY == platform_uart_read_async(&uart, buffer, sizeof(buffer)));

    return 0;
}

static int test_read_async_accepts_rx_event_raised_during_hal_start(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t buffer[256] = {0};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    g_fakeHalReceiveToIdleCallbackPosition = 10U;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, buffer, sizeof(buffer)));
    TEST_ASSERT(1U == record.eventCount);
    TEST_ASSERT(buffer == record.events[0].data);
    TEST_ASSERT(10U == record.events[0].dataLength);

    return 0;
}

static int test_rx_events_report_monotonic_wrap_and_duplicate_positions(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t buffer[256] = {0};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, buffer, sizeof(buffer)));

    HAL_UARTEx_RxEventCallback(&huart1, 10U);
    HAL_UARTEx_RxEventCallback(&huart1, 20U);
    HAL_UARTEx_RxEventCallback(&huart1, 20U);
    HAL_UARTEx_RxEventCallback(&huart1, 240U);
    HAL_UARTEx_RxEventCallback(&huart1, 20U);

    TEST_ASSERT(5U == record.eventCount);
    TEST_ASSERT(buffer == record.events[0].data);
    TEST_ASSERT(10U == record.events[0].dataLength);
    TEST_ASSERT(&buffer[10] == record.events[1].data);
    TEST_ASSERT(10U == record.events[1].dataLength);
    TEST_ASSERT(&buffer[20] == record.events[2].data);
    TEST_ASSERT(220U == record.events[2].dataLength);
    TEST_ASSERT(&buffer[240] == record.events[3].data);
    TEST_ASSERT(16U == record.events[3].dataLength);
    TEST_ASSERT(buffer == record.events[4].data);
    TEST_ASSERT(20U == record.events[4].dataLength);

    return 0;
}

static int test_tc_position_normalizes_and_does_not_duplicate_data(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t buffer[256] = {0};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, buffer, sizeof(buffer)));

    HAL_UARTEx_RxEventCallback(&huart1, 128U);
    HAL_UARTEx_RxEventCallback(&huart1, 128U);
    HAL_UARTEx_RxEventCallback(&huart1, 256U);
    HAL_UARTEx_RxEventCallback(&huart1, 0U);
    HAL_UARTEx_RxEventCallback(&huart1, 128U);

    TEST_ASSERT(3U == record.eventCount);
    TEST_ASSERT(buffer == record.events[0].data);
    TEST_ASSERT(128U == record.events[0].dataLength);
    TEST_ASSERT(&buffer[128] == record.events[1].data);
    TEST_ASSERT(128U == record.events[1].dataLength);
    TEST_ASSERT(buffer == record.events[2].data);
    TEST_ASSERT(128U == record.events[2].dataLength);

    return 0;
}

static int test_cancel_and_stop_abort_and_prevent_late_rx_events(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t buffer[256] = {0};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, buffer, sizeof(buffer)));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_cancel(&uart, PLATFORM_UART_DIRECTION_RX));
    TEST_ASSERT(1U == g_fakeHalAbortReceiveCallCount);
    TEST_ASSERT(1U == record.eventCount);
    TEST_ASSERT(PLATFORM_UART_EVENT_CANCELED == record.events[0].type);
    TEST_ASSERT(PLATFORM_UART_DIRECTION_RX == record.events[0].direction);
    TEST_ASSERT(PLATFORM_ERR_CANCELED == record.events[0].error);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, buffer, sizeof(buffer)));

    TEST_ASSERT(PLATFORM_ERR_OK == uart.device.lifecycle->stop(&uart));
    TEST_ASSERT(2U == g_fakeHalAbortReceiveCallCount);
    HAL_UARTEx_RxEventCallback(&huart1, 10U);
    TEST_ASSERT(1U == record.eventCount);

    return 0;
}

static int test_start_failures_and_rx_errors_release_session(void)
{
    platform_uart_t uart = PLATFORM_UART_INITIALIZER;
    event_record_t record = {0};
    uint8_t buffer[256] = {0};

    reset_fake_hal();
    TEST_ASSERT(0 == make_started_uart(&uart, &record));
    g_fakeHalReceiveToIdleResult = HAL_BUSY;
    TEST_ASSERT(PLATFORM_ERR_BUSY == platform_uart_read_async(&uart, buffer, sizeof(buffer)));
    g_fakeHalReceiveToIdleResult = HAL_ERROR;
    TEST_ASSERT(PLATFORM_ERR_IO == platform_uart_read_async(&uart, buffer, sizeof(buffer)));
    g_fakeHalReceiveToIdleResult = HAL_OK;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, buffer, sizeof(buffer)));

    huart1.ErrorCode = HAL_UART_ERROR_ORE;
    HAL_UART_ErrorCallback(&huart1);
    TEST_ASSERT(1U == record.eventCount);
    TEST_ASSERT(PLATFORM_UART_EVENT_ERROR == record.events[0].type);
    TEST_ASSERT(PLATFORM_UART_DIRECTION_RX == record.events[0].direction);
    TEST_ASSERT(PLATFORM_ERR_OVERFLOW == record.events[0].error);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_uart_read_async(&uart, buffer, sizeof(buffer)));

    return 0;
}

int main(void)
{
    int result = test_blocking_config_mappings_remain_supported();

    if (0 != result) {
        return result;
    }

    result = test_read_async_starts_dma_and_rejects_second_session();

    if (0 != result) {
        return result;
    }

    result = test_write_async_starts_tx_and_rejects_second_transaction();
    if (0 != result) {
        return result;
    }

    result = test_tx_complete_releases_transaction_before_notifying();
    if (0 != result) {
        return result;
    }

    result = test_cancel_tx_releases_transaction_and_notifies();
    if (0 != result) {
        return result;
    }

    result = test_cancel_both_terminates_independent_rx_and_tx_transactions();
    if (0 != result) {
        return result;
    }

    result = test_lifecycle_stop_terminates_tx_without_business_cancel_event();
    if (0 != result) {
        return result;
    }

    result = test_tx_dma_error_releases_tx_transaction();
    if (0 != result) {
        return result;
    }

    result = test_unattributed_dma_error_terminates_full_duplex_transactions();
    if (0 != result) {
        return result;
    }

    result = test_read_async_accepts_rx_event_raised_during_hal_start();
    if (0 != result) {
        return result;
    }

    result = test_rx_events_report_monotonic_wrap_and_duplicate_positions();
    if (0 != result) {
        return result;
    }

    result = test_tc_position_normalizes_and_does_not_duplicate_data();
    if (0 != result) {
        return result;
    }

    result = test_cancel_and_stop_abort_and_prevent_late_rx_events();
    if (0 != result) {
        return result;
    }

    return test_start_failures_and_rx_errors_release_session();
}
