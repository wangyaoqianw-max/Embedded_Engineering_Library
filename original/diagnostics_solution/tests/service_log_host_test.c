/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_service_log.c
 * @brief 验证 Service Log 公共接口、策略和日志参数转发。
 * @author YaoQian Wang
 * @date 2026-08-31
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#define LOG_TAG "service-log-test"
#include "service_log.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define TEST_ASSERT(condition)          \
    do {                                \
        if (!(condition)) {             \
            return __LINE__;            \
        }                               \
    } while (0)
//******************************** Defines *********************************//

//******************************** Types ***********************************//
typedef struct
{
    uint32_t initCallCount;
    uint32_t setLevelCallCount;
    uint32_t enableCallCount;
    uint32_t outputCallCount;
    platform_log_level_t level;
    uint8_t outputLevel;
    platform_bool_t enabled;
    const char *tag;
    const char *format;
    int argument;
} fake_platform_log_t;
//******************************** Types ***********************************//

//******************************** Variables ********************************//
static fake_platform_log_t g_log;
static platform_error_t g_initResult = PLATFORM_ERR_OK;
static platform_error_t g_setLevelResult = PLATFORM_ERR_OK;
static platform_error_t g_enableResult = PLATFORM_ERR_OK;
//******************************** Variables ********************************//

//******************************** Functions *********************************//
static void reset_fake(void)
{
    g_log = (fake_platform_log_t){0};
    g_initResult = PLATFORM_ERR_OK;
    g_setLevelResult = PLATFORM_ERR_OK;
    g_enableResult = PLATFORM_ERR_OK;
}

platform_error_t platform_log_init(void)
{
    g_log.initCallCount++;
    return g_initResult;
}

platform_error_t platform_log_set_level(platform_log_level_t level)
{
    g_log.setLevelCallCount++;
    g_log.level = level;
    return g_setLevelResult;
}

platform_error_t platform_log_enable_output(bool enable)
{
    g_log.enableCallCount++;
    g_log.enabled = (enable == true) ? PLATFORM_TRUE : PLATFORM_FALSE;
    return g_enableResult;
}

static void fake_output(uint8_t level,
                        const char *tag,
                        const char *file,
                        const char *func,
                        long line,
                        const char *format,
                        ...)
{
    va_list args;

    (void)file;
    (void)func;
    (void)line;

    va_start(args, format);
    g_log.outputLevel = level;
    g_log.tag = tag;
    g_log.format = format;
    if (0 == strcmp("value=%d", format)) {
        g_log.argument = va_arg(args, int);
    }
    g_log.outputCallCount++;
    va_end(args);
}

platform_log_output_fn_t platform_log_get_output_fn(void)
{
    return fake_output;
}

#if !defined(SERVICE_LOG_TEST_INIT_FAILURE) && \
    !defined(SERVICE_LOG_TEST_SET_LEVEL_FAILURE) && \
    !defined(SERVICE_LOG_TEST_ENABLE_FAILURE)
static int test_init_applies_default_policy(void)
{
    reset_fake();

    TEST_ASSERT(PLATFORM_ERR_OK == service_log_init());
    TEST_ASSERT(1U == g_log.initCallCount);
    TEST_ASSERT(1U == g_log.setLevelCallCount);
    TEST_ASSERT(PLATFORM_LOG_LEVEL_INFO == g_log.level);
    TEST_ASSERT(1U == g_log.enableCallCount);
    TEST_ASSERT(PLATFORM_TRUE == g_log.enabled);
    TEST_ASSERT(1U == g_log.outputCallCount);
    TEST_ASSERT(0 == strcmp("log service initialized", g_log.format));

    return 0;
}

static int test_invalid_level_rejected(void)
{
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                service_log_set_level(SERVICE_LOG_LEVEL_MAX));
    TEST_ASSERT(1U == g_log.setLevelCallCount);

    return 0;
}

static int test_level_mapping(void)
{
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_set_level(SERVICE_LOG_LEVEL_ERROR));
    TEST_ASSERT(PLATFORM_LOG_LEVEL_ERROR == g_log.level);
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_set_level(SERVICE_LOG_LEVEL_WARN));
    TEST_ASSERT(PLATFORM_LOG_LEVEL_WARN == g_log.level);
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_set_level(SERVICE_LOG_LEVEL_INFO));
    TEST_ASSERT(PLATFORM_LOG_LEVEL_INFO == g_log.level);
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_set_level(SERVICE_LOG_LEVEL_DEBUG));
    TEST_ASSERT(PLATFORM_LOG_LEVEL_DEBUG == g_log.level);
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_set_level(SERVICE_LOG_LEVEL_VERBOSE));
    TEST_ASSERT(PLATFORM_LOG_LEVEL_VERBOSE == g_log.level);

    return 0;
}

static int test_output_enable_forwards_to_platform(void)
{
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_enable_output(PLATFORM_FALSE));
    TEST_ASSERT(PLATFORM_FALSE == g_log.enabled);
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_enable_output(PLATFORM_TRUE));
    TEST_ASSERT(PLATFORM_TRUE == g_log.enabled);
    TEST_ASSERT(3U == g_log.enableCallCount);

    return 0;
}

static int test_successful_init_is_idempotent_after_runtime_changes(void)
{
    uint32_t initCallCount = g_log.initCallCount;
    uint32_t setLevelCallCount = g_log.setLevelCallCount;
    uint32_t enableCallCount = g_log.enableCallCount;

    TEST_ASSERT(PLATFORM_ERR_OK == service_log_set_level(SERVICE_LOG_LEVEL_DEBUG));
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_enable_output(PLATFORM_FALSE));
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_init());
    TEST_ASSERT(initCallCount == g_log.initCallCount);
    TEST_ASSERT((setLevelCallCount + 1U) == g_log.setLevelCallCount);
    TEST_ASSERT((enableCallCount + 1U) == g_log.enableCallCount);
    TEST_ASSERT(PLATFORM_LOG_LEVEL_DEBUG == g_log.level);
    TEST_ASSERT(PLATFORM_FALSE == g_log.enabled);

    initCallCount = g_log.initCallCount;
    setLevelCallCount = g_log.setLevelCallCount;
    enableCallCount = g_log.enableCallCount;
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_init());
    TEST_ASSERT(initCallCount == g_log.initCallCount);
    TEST_ASSERT(setLevelCallCount == g_log.setLevelCallCount);
    TEST_ASSERT(enableCallCount == g_log.enableCallCount);

    return 0;
}

static int test_log_macros_forward_context_and_arguments(void)
{
    SERVICE_LOG_E("value=%d", 1);
    TEST_ASSERT(PLATFORM_LOG_LEVEL_ERROR == g_log.outputLevel);
    SERVICE_LOG_W("value=%d", 2);
    TEST_ASSERT(PLATFORM_LOG_LEVEL_WARN == g_log.outputLevel);
    SERVICE_LOG_I("value=%d", 3);
    TEST_ASSERT(PLATFORM_LOG_LEVEL_INFO == g_log.outputLevel);
    SERVICE_LOG_D("value=%d", 4);
    TEST_ASSERT(PLATFORM_LOG_LEVEL_DEBUG == g_log.outputLevel);
    SERVICE_LOG_V("value=%d", 5);
    TEST_ASSERT(PLATFORM_LOG_LEVEL_VERBOSE == g_log.outputLevel);
    TEST_ASSERT(0 == strcmp("service-log-test", g_log.tag));
    TEST_ASSERT(0 == strcmp("value=%d", g_log.format));
    TEST_ASSERT(5 == g_log.argument);

    return 0;
}
#endif

#if defined(SERVICE_LOG_TEST_INIT_FAILURE)
int main(void)
{
    reset_fake();
    g_initResult = PLATFORM_ERR_IO;
    TEST_ASSERT(PLATFORM_ERR_IO == service_log_init());
    TEST_ASSERT(1U == g_log.initCallCount);
    TEST_ASSERT(0U == g_log.setLevelCallCount);
    TEST_ASSERT(0U == g_log.enableCallCount);
    TEST_ASSERT(0U == g_log.outputCallCount);

    g_initResult = PLATFORM_ERR_OK;
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_init());
    TEST_ASSERT(2U == g_log.initCallCount);
    TEST_ASSERT(1U == g_log.setLevelCallCount);
    TEST_ASSERT(1U == g_log.enableCallCount);
    TEST_ASSERT(1U == g_log.outputCallCount);

    return 0;
}
#elif defined(SERVICE_LOG_TEST_SET_LEVEL_FAILURE)
int main(void)
{
    reset_fake();
    g_setLevelResult = PLATFORM_ERR_IO;
    TEST_ASSERT(PLATFORM_ERR_IO == service_log_init());
    TEST_ASSERT(1U == g_log.initCallCount);
    TEST_ASSERT(1U == g_log.setLevelCallCount);
    TEST_ASSERT(0U == g_log.enableCallCount);
    TEST_ASSERT(0U == g_log.outputCallCount);

    g_setLevelResult = PLATFORM_ERR_OK;
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_init());
    TEST_ASSERT(2U == g_log.initCallCount);
    TEST_ASSERT(2U == g_log.setLevelCallCount);
    TEST_ASSERT(1U == g_log.enableCallCount);
    TEST_ASSERT(1U == g_log.outputCallCount);

    return 0;
}
#elif defined(SERVICE_LOG_TEST_ENABLE_FAILURE)
int main(void)
{
    reset_fake();
    g_enableResult = PLATFORM_ERR_IO;
    TEST_ASSERT(PLATFORM_ERR_IO == service_log_init());
    TEST_ASSERT(1U == g_log.initCallCount);
    TEST_ASSERT(1U == g_log.setLevelCallCount);
    TEST_ASSERT(1U == g_log.enableCallCount);
    TEST_ASSERT(0U == g_log.outputCallCount);

    g_enableResult = PLATFORM_ERR_OK;
    TEST_ASSERT(PLATFORM_ERR_OK == service_log_init());
    TEST_ASSERT(2U == g_log.initCallCount);
    TEST_ASSERT(2U == g_log.setLevelCallCount);
    TEST_ASSERT(2U == g_log.enableCallCount);
    TEST_ASSERT(1U == g_log.outputCallCount);

    return 0;
}
#else
int main(void)
{
    int result = test_init_applies_default_policy();

    if (0 != result) {
        return result;
    }

    result = test_invalid_level_rejected();
    if (0 != result) {
        return result;
    }

    result = test_level_mapping();
    if (0 != result) {
        return result;
    }

    result = test_output_enable_forwards_to_platform();
    if (0 != result) {
        return result;
    }

    result = test_successful_init_is_idempotent_after_runtime_changes();
    if (0 != result) {
        return result;
    }

    return test_log_macros_forward_context_and_arguments();
}
#endif
