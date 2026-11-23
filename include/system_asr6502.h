/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * CMSIS system interface for the ASR6502 SoC.
 */

#ifndef SYSTEM_ASR6502_H
#define SYSTEM_ASR6502_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Current core clock frequency in Hz; refreshed by SystemCoreClockUpdate(). */
extern uint32_t SystemCoreClock;

/* Bring the core up to a defined state. Called from the reset handler before main(). */
void SystemInit(void);

/* Recompute SystemCoreClock from the active clock configuration. */
void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_ASR6502_H */
