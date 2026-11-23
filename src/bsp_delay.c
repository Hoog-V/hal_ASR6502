/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Software (busy-loop) delay for the ASR6502.
 */

#include "bsp_delay.h"
#include "asr6502.h"

/*
 * CPU cycles consumed by one pass of the inner delay loop (SUBS = 1, taken
 * branch = 2 on Cortex-M0+). Flash wait states can shift the real figure; tune
 * this if measured delays are off.
 */
#ifndef ASR_DELAY_CYCLES_PER_LOOP
 #define ASR_DELAY_CYCLES_PER_LOOP    (3U)
#endif

void asr_delay_cycles(uint32_t cycles)
{
    uint32_t loops = cycles / ASR_DELAY_CYCLES_PER_LOOP;

    if (loops == 0U) {
        return;
    }

    __asm volatile (
        "1: subs %0, %0, #1 \n"
        "   bne  1b         \n"
        : "+r" (loops)
        :
        : "cc");
}

void asr_delay(uint32_t delay, asr_delay_units_t units)
{
    /* Requested time in microseconds (fits in 64 bits for any uint32 delay). */
    uint64_t total_us = (uint64_t) delay * (uint64_t) units;

    /* Cycles per microsecond from the live core clock. */
    uint32_t cycles_per_us = SystemCoreClock / 1000000U;
    uint64_t cycles        = total_us * (uint64_t) cycles_per_us;

    /* asr_delay_cycles() takes a 32-bit count; stage very long delays. */
    while (cycles > (uint64_t) UINT32_MAX) {
        asr_delay_cycles(UINT32_MAX);
        cycles -= (uint64_t) UINT32_MAX;
    }

    asr_delay_cycles((uint32_t) cycles);
}
