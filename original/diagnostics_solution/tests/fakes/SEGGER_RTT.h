/******************************************************************************
 * @file SEGGER_RTT.h
 * @brief Platform Log Host Test 使用的最小 RTT fake 声明。
 * @author YaoQian Wang
 * @date 2026-08-31
 * @version V1.0
 *****************************************************************************/

#ifndef TEST_PLATFORM_LOG_FAKE_SEGGER_RTT_H
#define TEST_PLATFORM_LOG_FAKE_SEGGER_RTT_H

unsigned SEGGER_RTT_WriteString(unsigned BufferIndex, const char *s);
int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...);

#endif
