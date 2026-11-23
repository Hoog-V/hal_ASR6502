/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NVIC helpers for the ASR6502.
 */

#include "bsp_irq.h"

/* Driver context per device interrupt line. */
static void *s_isr_context[ASR_IRQ_LINE_COUNT];

void asr_irq_context_set(IRQn_Type irq, void *context)
{
    if ((uint32_t) irq < ASR_IRQ_LINE_COUNT) {
        s_isr_context[(uint32_t) irq] = context;
    }
}

void *asr_irq_context_get(IRQn_Type irq)
{
    if ((uint32_t) irq < ASR_IRQ_LINE_COUNT) {
        return s_isr_context[(uint32_t) irq];
    }

    return (void *) 0;
}

void asr_irq_clear_pending(IRQn_Type irq)
{
    NVIC_ClearPendingIRQ(irq);
}

void asr_irq_cfg(IRQn_Type irq, uint32_t priority, void *context)
{
    NVIC_SetPriority(irq, priority);
    asr_irq_context_set(irq, context);
}

void asr_irq_enable(IRQn_Type irq)
{
    NVIC_ClearPendingIRQ(irq);
    NVIC_EnableIRQ(irq);
}

void asr_irq_disable(IRQn_Type irq)
{
    NVIC_DisableIRQ(irq);
    __DSB();
    __ISB();
}

void asr_irq_cfg_enable(IRQn_Type irq, uint32_t priority, void *context)
{
    asr_irq_cfg(irq, priority, context);
    asr_irq_enable(irq);
}
