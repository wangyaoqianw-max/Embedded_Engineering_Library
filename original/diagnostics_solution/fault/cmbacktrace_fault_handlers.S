;******************************************************************************
;* Copyright (C) 2026 YaoQian Wang                                            *
;*                                                                            *
;* All Rights Reserved.                                                       *
;*                                                                            *
;* @file cmbacktrace_fault_handlers.S                                         *
;* @brief 将 Cortex-M Fault 统一转交给工程诊断和 CmBacktrace。                *
;* @author YaoQian Wang                                                       *
;* @date 2026-09-15                                                          *
;* @version V1.0                                                             *
;******************************************************************************

    AREA |.text|, CODE, READONLY, ALIGN=2
    THUMB
    REQUIRE8
    PRESERVE8

    IMPORT diagnostics_fault_handler
    EXPORT HardFault_Handler
    EXPORT MemManage_Handler
    EXPORT BusFault_Handler
    EXPORT UsageFault_Handler
    EXPORT diagnostics_fault_capture_stop

HardFault_Handler     PROC
    B       diagnostics_fault_entry
    ENDP

MemManage_Handler    PROC
    B       diagnostics_fault_entry
    ENDP

BusFault_Handler     PROC
    B       diagnostics_fault_entry
    ENDP

UsageFault_Handler   PROC
    B       diagnostics_fault_entry
    ENDP

diagnostics_fault_entry PROC
    MOV     r0, lr
    TST     r0, #4
    ITE     EQ
    MRSEQ   r1, MSP
    MRSNE   r1, PSP
    BL      diagnostics_fault_handler

diagnostics_fault_capture_stop
Fault_Loop
    B       Fault_Loop
    ENDP

    END
