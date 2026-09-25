/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_platform_log.c
 * @brief 验证 Platform Log 公共 API 与 EasyLogger 适配契约。
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.0
 *
 *****************************************************************************/

#include <stdarg.h>
#include <string.h>

#define LOG_TAG "platform-log-test"

#include "SEGGER_RTT.h"
#include "cmsis_os.h"
#include "elog.h"
#include "platform_log.h"
#include "task.h"

#define TEST_ASSERT(condition)          \
    do {                                \
        if (!(condition)) {             \
            return __LINE__;            \
        }                               \
    } while (0)

typedef struct
{
    uint8_t level;
    const char *tag;
    const char *file;
    const char *func;
    long line;
    const char *format;
    int argument;
    uint32_t outputCallCount;
    uint32_t initCallCount;
    uint32_t startCallCount;
    uint32_t setFormatCallCount;
    uint32_t setLevelCallCount;
    uint32_t setOutputEnabledCallCount;
    uint8_t configuredLevel;
    bool outputEnabled;
} fake_log_runtime_t;

static fake_log_runtime_t g_logRuntime;

ElogErrCode elog_init(void)
{
    g_logRuntime.initCallCount++;
    return ELOG_NO_ERR;
}

void elog_start(void)
{
    g_logRuntime.startCallCount++;
}

void elog_set_output_enabled(bool enabled)
{
    g_logRuntime.outputEnabled = enabled;
    g_logRuntime.setOutputEnabledCallCount++;
}

void elog_set_fmt(uint8_t level, size_t set)
{
    (void)level;
    (void)set;
    g_logRuntime.setFormatCallCount++;
}

void elog_set_filter_lvl(uint8_t level)
{
    g_logRuntime.configuredLevel = level;
    g_logRuntime.setLevelCallCount++;
}

void elog_output(uint8_t level, const char *tag, const char *file, const char *func,
                 const long line, const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    g_logRuntime.level = level;
    g_logRuntime.tag = tag;
    g_logRuntime.file = file;
    g_logRuntime.func = func;
    g_logRuntime.line = line;
    g_logRuntime.format = format;
    g_logRuntime.argument = va_arg(arguments, int);
    g_logRuntime.outputCallCount++;
    va_end(arguments);
}

void elog_assert_set_hook(void (*hook)(const char *expr, const char *func, size_t line))
{
    (void)hook;
}

void elog_port_output(const char *log, size_t size)
{
    (void)log;
    (void)size;
}

size_t elog_async_get_line_log(char *log, size_t size)
{
    (void)log;
    (void)size;
    return 0U;
}

osSemaphoreId_t osSemaphoreNew(uint32_t maximumCount, uint32_t initialCount,
                                const osSemaphoreAttr_t *attributes)
{
    (void)maximumCount;
    (void)initialCount;
    (void)attributes;
    return (osSemaphoreId_t)1;
}

osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphoreId, uint32_t timeout)
{
    (void)semaphoreId;
    (void)timeout;
    return osErrorResource;
}

osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphoreId)
{
    (void)semaphoreId;
    return osOK;
}

osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphoreId)
{
    (void)semaphoreId;
    return osOK;
}

osThreadId_t osThreadNew(osThreadFunc_t function, void *argument,
                         const osThreadAttr_t *attributes)
{
    (void)function;
    (void)argument;
    (void)attributes;
    return (osThreadId_t)1;
}

osStatus_t osThreadTerminate(osThreadId_t threadId)
{
    (void)threadId;
    return osOK;
}

unsigned SEGGER_RTT_WriteString(unsigned BufferIndex, const char *s)
{
    (void)BufferIndex;
    (void)s;
    return 0U;
}

int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...)
{
    (void)BufferIndex;
    (void)sFormat;
    return 0;
}

void vTaskDelete(TaskHandle_t task)
{
    (void)task;
}

static int test_pre_init_getter_returns_safe_backend(void)
{
    platform_log_output_fn_t output = platform_log_get_output_fn();

    TEST_ASSERT(NULL != output);
    output((uint8_t)PLATFORM_LOG_LEVEL_INFO, LOG_TAG, __FILE__, __func__, __LINE__, "value=%d", 42);
    platform_log_i("value=%d", 42);
    TEST_ASSERT(0U == g_logRuntime.outputCallCount);

    return 0;
}

static int test_pre_init_control_apis_report_not_initialized(void)
{
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED == platform_log_set_level(PLATFORM_LOG_LEVEL_INFO));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED == platform_log_enable_output(false));

    return 0;
}

static int test_init_repeat_and_active_backend(void)
{
    TEST_ASSERT(PLATFORM_ERR_OK == platform_log_init());
    TEST_ASSERT(1U == g_logRuntime.initCallCount);
    TEST_ASSERT(1U == g_logRuntime.startCallCount);
    TEST_ASSERT(6U == g_logRuntime.setFormatCallCount);
    TEST_ASSERT(elog_output == platform_log_get_output_fn());
    TEST_ASSERT(PLATFORM_ERR_OK == platform_log_init());
    TEST_ASSERT(1U == g_logRuntime.initCallCount);

    platform_log_i("value=%d", 42);

    TEST_ASSERT(1U == g_logRuntime.outputCallCount);
    TEST_ASSERT(PLATFORM_LOG_LEVEL_INFO == g_logRuntime.level);
    TEST_ASSERT(0 == strcmp(LOG_TAG, g_logRuntime.tag));
    TEST_ASSERT(0 == strcmp("value=%d", g_logRuntime.format));
    TEST_ASSERT(42 == g_logRuntime.argument);
    TEST_ASSERT(NULL != g_logRuntime.file);
    TEST_ASSERT(NULL != g_logRuntime.func);
    TEST_ASSERT(0L < g_logRuntime.line);

    return 0;
}

static int test_level_and_output_controls_preserve_backend_mapping(void)
{
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM == platform_log_set_level(PLATFORM_LOG_LEVEL_MAX));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_log_set_level(PLATFORM_LOG_LEVEL_INFO));
    TEST_ASSERT(1U == g_logRuntime.setLevelCallCount);
    TEST_ASSERT(ELOG_LVL_INFO == g_logRuntime.configuredLevel);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_log_enable_output(false));
    TEST_ASSERT(false == g_logRuntime.outputEnabled);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_log_enable_output(true));
    TEST_ASSERT(true == g_logRuntime.outputEnabled);
    TEST_ASSERT(2U == g_logRuntime.setOutputEnabledCallCount);

    return 0;
}

int main(void)
{
    int result = test_pre_init_getter_returns_safe_backend();

    if (0 != result) {
        return result;
    }

    result = test_pre_init_control_apis_report_not_initialized();
    if (0 != result) {
        return result;
    }

    result = test_init_repeat_and_active_backend();
    if (0 != result) {
        return result;
    }

    return test_level_and_output_controls_preserve_backend_mapping();
}
