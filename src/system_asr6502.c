/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * CMSIS system implementation for the ASR6502 SoC.
 *
 * SystemInit() relocates the vector table and refreshes SystemCoreClock from the
 * live clock registers. The IMO starts at its SFLASH-trimmed default; change it
 * with asr_clock_imo_set_freq() (bsp_clocks).
 */

#include "asr6502.h"
#include "bsp_clocks.h"

/* Live core clock in Hz. */
uint32_t SystemCoreClock = ASR_DEFAULT_CORE_CLOCK_HZ;

void SystemCoreClockUpdate(void)
{
    /* Recompute SystemCoreClock from the live SRSSLT clock registers. */
    asr_clock_sysclk_update();
}

void SystemInit(void)
{
    /* Relocate the vector table to its link address (flash or SRAM copy). */
#if defined(__VTOR_PRESENT) && (__VTOR_PRESENT == 1U)
    extern uint32_t __vector_table[];
    SCB->VTOR = (uint32_t) __vector_table;
#endif

    /* The boot ROM leaves the IMO at its SFLASH-trimmed default; use
     * asr_clock_imo_set_freq() to retune it. */

    SystemCoreClockUpdate();
}
