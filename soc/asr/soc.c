/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ASR6502 SoC early init: the boot ROM leaves the IMO at its 24 MHz default.
 * Retrim it to 48 MHz to match CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC; otherwise the
 * kernel clock runs 2x slow in real time and time-critical work such as LoRaWAN
 * RX windows fires at the wrong moment. Must happen before SysTick starts.
 */

#include <zephyr/kernel.h>

#include "bsp_clocks.h"

void soc_early_init_hook(void)
{
	asr_clock_imo_set_freq(ASR_CLOCK_IMO_48MHZ);
	asr_clock_sysclk_update();
}
