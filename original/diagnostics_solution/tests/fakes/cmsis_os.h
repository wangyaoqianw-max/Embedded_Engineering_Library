/******************************************************************************
 * @file cmsis_os.h
 * @brief Platform Log Host Test 使用的最小 CMSIS-RTOS fake 声明。
 * @author YaoQian Wang
 * @date 2026-08-31
 * @version V1.0
 *****************************************************************************/

#ifndef TEST_PLATFORM_LOG_FAKE_CMSIS_OS_H
#define TEST_PLATFORM_LOG_FAKE_CMSIS_OS_H

#include <stdint.h>

#define osWaitForever 0xFFFFFFFFU

typedef void *osSemaphoreId_t;
typedef void *osThreadId_t;
typedef void (*osThreadFunc_t)(void *argument);

typedef enum
{
    osOK = 0,
    osErrorResource = -3
} osStatus_t;

typedef enum
{
    osPriorityBelowNormal = -1
} osPriority_t;

typedef struct
{
    const char *name;
    uint32_t stack_size;
    osPriority_t priority;
} osThreadAttr_t;

typedef struct
{
    uint32_t reserved;
} osSemaphoreAttr_t;

osSemaphoreId_t osSemaphoreNew(uint32_t maximumCount,
                                uint32_t initialCount,
                                const osSemaphoreAttr_t *attributes);
osStatus_t osSemaphoreAcquire(osSemaphoreId_t semaphoreId, uint32_t timeout);
osStatus_t osSemaphoreRelease(osSemaphoreId_t semaphoreId);
osStatus_t osSemaphoreDelete(osSemaphoreId_t semaphoreId);
osThreadId_t osThreadNew(osThreadFunc_t function,
                         void *argument,
                         const osThreadAttr_t *attributes);
osStatus_t osThreadTerminate(osThreadId_t threadId);

#endif
