/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file easylogger_port.c
 * @brief 日志集中管理头文件
 * @author YaoQian Wang
 * @date 2026-05-09
 * @version V1.0
 * @note
 * @warning
 * @history
 * 1. 2026-05-09 创建项目
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "easylogger_port.h"
#include "platform_log.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define IMPL_ELOG_ASYNC_SEM_MAX_COUNT    (16U)
#define IMPL_ELOG_ASYNC_TASK_STACK_SIZE  (4096U)

//初始化状态，默认为false，初始化成功置true
static bool g_appLogInited = false;

/**
 * @brief  日志未初始化时丢弃输出
 */
static void impl_elog_no_output(uint8_t level,
                               const char *tag,
                               const char *file,
                               const char *func,
                               long line,
                               const char *format,
                               ...);
static platform_log_output_fn_t g_logOutputFn = impl_elog_no_output;

/**
 * @brief  各个级别的日志输出内容设置
 */
static void impl_elog_config_format(void);

/**
 * @brief  将app日志等级转换为elog日志等级
 * @param  level platform_log的日志等级
 * @return uint8_t 返回platform_log枚举类型对应的elog日志等级
 * @note   可以根据日志中间件的日志等级进行相应的映射
 */
static uint8_t impl_elog_convert_level(platform_log_level_t level);

/**
  * @brief  钩子函数：打印断言表达式；打印触发函数；打印触发行号
  * @brief            关中断；停机等待调试器接管
  * @param  expr 断言表达式
  * @param  func 触发错误的函数
  * @param  line 在第几行触发的错误
  */
static void impl_elog_assert_hook(const char *expr,const char *func,size_t line);

/*异步模式*/
#if defined(ELOG_ASYNC_OUTPUT_ENABLE)
//创建信号量的句柄
static osSemaphoreId_t g_elogAsyncSemHandle;
//创建日志任务的句柄
static osThreadId_t g_elogAsyncTaskHandle;
static const osThreadAttr_t s_elog_async_task_attributes = {
    .name = "elogAsync",
    .stack_size = IMPL_ELOG_ASYNC_TASK_STACK_SIZE,
    .priority = (osPriority_t) osPriorityBelowNormal,
};

/**
 * @brief  如果是异步模式，需要将elog_port.c中的该函数进行显式声明
 */
extern void elog_port_output(const char *log, size_t size);

/**
 * @brief  异步模式，日志任务函数，用来接收信号量，触发日志传输
 */
static void impl_elog_async_task(void *argument);
#endif
//******************************** Defines *********************************//

//******************************** Function *********************************//
/**
 * @brief  对日志进行初始化
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败
 */
platform_error_t platform_log_init(void)
{
    platform_error_t result = PLATFORM_ERR_OK;
    //防止重复初始化
    if (g_appLogInited == true) {
        return PLATFORM_ERR_OK;
    }

    //如果是异步状态，需要创建信号量和任务
#if defined(ELOG_ASYNC_OUTPUT_ENABLE)
    g_elogAsyncSemHandle = osSemaphoreNew(IMPL_ELOG_ASYNC_SEM_MAX_COUNT, 0, NULL);
    if (g_elogAsyncSemHandle == NULL) {
        return PLATFORM_ERR_NO_RESOURCE;
    }
    g_elogAsyncTaskHandle = osThreadNew(impl_elog_async_task, NULL, &s_elog_async_task_attributes);
    if (g_elogAsyncTaskHandle == NULL) {
        result = PLATFORM_ERR_NO_RESOURCE;
        goto init_failed;
    }
#endif

    //日志初始化
    if(ELOG_NO_ERR != elog_init()){
        result = PLATFORM_ERR_IO;
        goto init_failed;
    }
    //日志钩子函数注册
    elog_assert_set_hook(impl_elog_assert_hook);
    //对日志输出项进行配置
    impl_elog_config_format();
    //日志开始运行
    elog_start();
    g_appLogInited = true;
    g_logOutputFn = elog_output;
    return PLATFORM_ERR_OK;

init_failed:
//异步模式下如果初始化失败，释放信号量和线程申请的资源
#if defined(ELOG_ASYNC_OUTPUT_ENABLE)
    if (g_elogAsyncTaskHandle != NULL) {
        osThreadTerminate(g_elogAsyncTaskHandle);
        g_elogAsyncTaskHandle = NULL;
    }

    if (g_elogAsyncSemHandle != NULL) {
        osSemaphoreDelete(g_elogAsyncSemHandle);
        g_elogAsyncSemHandle = NULL;
    }
#endif
    g_appLogInited = false;
    g_logOutputFn = impl_elog_no_output;
    return result;
}
/**
 * @brief  设置可以输出的日志的级别
 * @param  level  代表要设置的日志级别
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败
 */
platform_error_t platform_log_set_level(platform_log_level_t level)
{
    //初始化和参数校验
    if (!g_appLogInited) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (level >= PLATFORM_LOG_LEVEL_MAX) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    elog_set_filter_lvl(impl_elog_convert_level(level));
    return PLATFORM_ERR_OK;
}
/**
 * @brief  打开/关闭日志输出
 * @param  enable  代表日志的开关指令
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败
 */
platform_error_t platform_log_enable_output(bool enable)
{
    if (!g_appLogInited) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    elog_set_output_enabled(enable);
    return PLATFORM_ERR_OK;
}

platform_log_output_fn_t platform_log_get_output_fn(void)
{
    return g_logOutputFn;
}

#if defined(ELOG_ASYNC_OUTPUT_ENABLE)
/**
 * @brief  将elog_async.c中显式声明的函数进行实现，功能是异步模式发送信号量
 */
void elog_async_output_notice(void)
{
    if (g_elogAsyncSemHandle != NULL) {
        osSemaphoreRelease(g_elogAsyncSemHandle);
    }
}
#endif

//********************* 私有函数 ********************//
static void impl_elog_no_output(uint8_t level,
                               const char *tag,
                               const char *file,
                               const char *func,
                               long line,
                               const char *format,
                               ...)
{
    (void)level;
    (void)tag;
    (void)file;
    (void)func;
    (void)line;
    (void)format;
}

#if defined(ELOG_ASYNC_OUTPUT_ENABLE)
/**
 * @brief  异步模式，日志任务函数，用来接收信号量，触发日志传输
 */
static void impl_elog_async_task(void *argument)
{
    static char s_async_log_buf[ELOG_LINE_BUF_SIZE];
    size_t log_size;

    for (;;) {
        if (osSemaphoreAcquire(g_elogAsyncSemHandle, osWaitForever) != osOK) {
            SEGGER_RTT_WriteString(0U, "Semaphore failed. Task delete.\r\n");
            vTaskDelete(NULL);
        }

        do {
            log_size = elog_async_get_line_log(s_async_log_buf, sizeof(s_async_log_buf));

            if (log_size > 0) {
                elog_port_output(s_async_log_buf, log_size);
            }
        } while (log_size > 0);
    }
}
#endif

/**
 * @brief  各个级别的日志输出内容设置
 */
static void impl_elog_config_format(void)
{
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);

    elog_set_fmt(ELOG_LVL_ERROR,
               ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_T_INFO);

    elog_set_fmt(ELOG_LVL_WARN,
               ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_T_INFO);

    elog_set_fmt(ELOG_LVL_INFO,
               ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_T_INFO);

    elog_set_fmt(ELOG_LVL_DEBUG,
               ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_T_INFO);

    elog_set_fmt(ELOG_LVL_VERBOSE,
               ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_T_INFO);
}

/**
 * @brief  将app日志等级转换为elog日志等级
 * @param  level platform_log的日志等级
 * @return uint8_t 返回platform_log枚举类型对应的elog日志等级
 * @note   可以根据日志中间件的日志等级进行相应的映射
 */
static uint8_t impl_elog_convert_level(platform_log_level_t level)
{
    switch (level) {
        case PLATFORM_LOG_LEVEL_ASSERT:
            return ELOG_LVL_ASSERT;

        case PLATFORM_LOG_LEVEL_ERROR:
            return ELOG_LVL_ERROR;

        case PLATFORM_LOG_LEVEL_WARN:
            return ELOG_LVL_WARN;

        case PLATFORM_LOG_LEVEL_INFO:
            return ELOG_LVL_INFO;

        case PLATFORM_LOG_LEVEL_DEBUG:
            return ELOG_LVL_DEBUG;

        case PLATFORM_LOG_LEVEL_VERBOSE:
            return ELOG_LVL_VERBOSE;

        default:
            return ELOG_LVL_ERROR;
    }
}

/**
  * @brief  钩子函数：打印断言表达式；打印触发函数；打印触发行号
  * @brief            关中断；停机等待调试器接管
  * @param  expr 断言表达式
  * @param  func 触发错误的函数
  * @param  line 在第几行触发的错误
  */
static void impl_elog_assert_hook(const char *expr, const char *func, size_t line)
{
    SEGGER_RTT_WriteString(0U, "\r\n[ASSERT] EasyLogger assert failed\r\n");
    SEGGER_RTT_WriteString(0U, "expr: ");
    SEGGER_RTT_WriteString(0U, expr);
    SEGGER_RTT_WriteString(0U, "\r\nfunc: ");
    SEGGER_RTT_WriteString(0U, func);
    SEGGER_RTT_WriteString(0U, "\r\nline: ");

    SEGGER_RTT_printf(0U, "%lu\r\n", (unsigned long)line);

    taskDISABLE_INTERRUPTS();

    while (1) {
    }
}
//********************* 私有函数 ********************//

//******************************** Function *********************************//
