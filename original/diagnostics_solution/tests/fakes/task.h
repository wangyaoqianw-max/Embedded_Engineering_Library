/******************************************************************************
 * @file task.h
 * @brief Platform Log Host Test 使用的最小 FreeRTOS task fake 声明。
 * @author YaoQian Wang
 * @date 2026-08-31
 * @version V1.0
 *****************************************************************************/

#ifndef TEST_PLATFORM_LOG_FAKE_TASK_H
#define TEST_PLATFORM_LOG_FAKE_TASK_H

typedef void *TaskHandle_t;

#define taskDISABLE_INTERRUPTS() ((void)0)

void vTaskDelete(TaskHandle_t task);

#endif
