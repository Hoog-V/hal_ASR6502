/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ASR6502 SoC early init: the boot ROM leaves the IMO at its SFLASH-trimmed
 * default, so here we only refresh SystemCoreClock from the live clock registers.
 */

#include <zephyr/kernel.h>

#include "bsp_clocks.h"

void soc_early_init_hook(void)
{
	asr_clock_sysclk_update();
}
