/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Software (busy-loop) delay for the ASR6502. Timing is derived from
 * SystemCoreClock, so SystemCoreClockUpdate() must reflect the active clock.
 */

#ifndef BSP_DELAY_H
#define BSP_DELAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Scale factor that turns a delay argument into microseconds. */
typedef enum
{
    ASR_DELAY_UNITS_SECONDS      = 1000000, /* Argument is in seconds */
    ASR_DELAY_UNITS_MILLISECONDS = 1000,    /* Argument is in milliseconds */
    ASR_DELAY_UNITS_MICROSECONDS = 1,       /* Argument is in microseconds */
} asr_delay_units_t;

/* Busy-wait for (delay * units) microseconds. */
void asr_delay(uint32_t delay, asr_delay_units_t units);

/* Busy-wait for approximately the given number of CPU cycles. */
void asr_delay_cycles(uint32_t cycles);

#ifdef __cplusplus
}
#endif

#endif /* BSP_DELAY_H */
