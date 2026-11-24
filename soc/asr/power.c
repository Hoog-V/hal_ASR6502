/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Power management for the ASR6502. Zephyr PM states map to the SoC operating
 * modes:
 *   - PM_STATE_SUSPEND_TO_IDLE -> Sleep      (CPU clock gated, clocks run,
 *                                             wake on any interrupt)
 *   - PM_STATE_STANDBY         -> Deep Sleep (high-frequency clocks stop, wake
 *                                             from a deep-sleep source: GPIO
 *                                             ports 0..3, watchdog)
 * Active mode is the normal running state.
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <cmsis_core.h>

#include "bsp_clocks.h"

/* Enter the low-power state. Called by the idle thread with interrupts locked. */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
		break;
	case PM_STATE_STANDBY:
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		break;
	default:
		return;
	}

	__DSB();
	__WFI();
}

/* Restore after waking. Called with interrupts still locked; re-enables them. */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	if (state == PM_STATE_STANDBY) {
		/* The IMO is restarted by hardware on deep-sleep exit; make the
		 * cached core-clock rate reflect the live configuration again. */
		SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
		asr_clock_sysclk_update();
	}

	__enable_irq();
	__ISB();
}
