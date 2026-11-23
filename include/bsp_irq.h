/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NVIC helpers for the ASR6502: thin wrappers over CMSIS NVIC plus a per-line
 * context table so an ISR can recover the driver instance that owns it.
 */

#ifndef BSP_IRQ_H
#define BSP_IRQ_H

#include <stdint.h>
#include "asr6502.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Store/fetch the driver context for a device interrupt line (0..ASR_IRQ_LINE_COUNT-1). */
void  asr_irq_context_set(IRQn_Type irq, void *context);
void *asr_irq_context_get(IRQn_Type irq);

/* Clear a pending interrupt in the NVIC. */
void asr_irq_clear_pending(IRQn_Type irq);

/* Set priority and store the context, without enabling the line. */
void asr_irq_cfg(IRQn_Type irq, uint32_t priority, void *context);

/* Clear any stale pending state and enable the line in the NVIC. */
void asr_irq_enable(IRQn_Type irq);

/* Disable the line in the NVIC and synchronise. */
void asr_irq_disable(IRQn_Type irq);

/* asr_irq_cfg() followed by asr_irq_enable(). */
void asr_irq_cfg_enable(IRQn_Type irq, uint32_t priority, void *context);

#ifdef __cplusplus
}
#endif

#endif /* BSP_IRQ_H */
