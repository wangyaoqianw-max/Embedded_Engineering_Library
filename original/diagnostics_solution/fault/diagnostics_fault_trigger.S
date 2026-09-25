;******************************************************************************
; * Copyright (C) 2026 YaoQian Wang
; *
; * All Rights Reserved.
; *
; * @file diagnostics_fault_trigger.S
; * @brief Cortex-M undefined-instruction Fault trigger.
; * @author YaoQian Wang
; * @date 2026-09-15
; * @version V1.0
;******************************************************************************

        AREA    |.text|, CODE, READONLY, ALIGN=2
        THUMB
        REQUIRE8
        PRESERVE8

        EXPORT  diagnostics_fault_trigger_undefined [CODE]

diagnostics_fault_trigger_undefined PROC
        NOP
        ; 0xDEAD is the Thumb UDF encoding used by DIAG_FAULT_UNDEFINED_OPCODE.
        DCW     0xDEAD
        BX      lr
        ENDP

        END
