/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Pin controller for the ASR6502: routes a pin to a peripheral via HSIOM and
 * sets its GPIO drive mode. Pins are described entirely in devicetree.
 */

#include <zephyr/drivers/pinctrl.h>

#include "asr6502.h"
#include "bsp_io.h"   /* ASR_GPIO_DM_* */

#define REG(addr) (*(volatile uint32_t *)(addr))

static uint32_t asr_drive_mode(uint8_t flags)
{
	if (flags & ASR_PINCFG_INPUT) {
		return ASR_GPIO_DM_INPUT;
	}
	if (flags & ASR_PINCFG_PULLUP) {
		return ASR_GPIO_DM_PULLUP;
	}
	if (flags & ASR_PINCFG_PULLDOWN) {
		return ASR_GPIO_DM_PULLDOWN;
	}
	if (flags & ASR_PINCFG_OPENDRAIN) {
		return ASR_GPIO_DM_OD_DRIVES_LOW;
	}

	return ASR_GPIO_DM_STRONG;
}

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		const pinctrl_soc_pin_t *p = &pins[i];
		uint32_t dm = asr_drive_mode(p->flags);
		uintptr_t hsiom = ASR_HSIOM_PRT_BASE(p->port) + ASR_HSIOM_PORT_SEL0;
		uintptr_t pc = ASR_GPIO_PRT_BASE(p->port) + ASR_GPIO_PC;

		REG(hsiom) = (REG(hsiom) & ~(0xFUL << (p->pin * 4))) |
			     ((uint32_t)p->hsiom << (p->pin * 4));
		REG(pc) = (REG(pc) & ~(0x7UL << (p->pin * 3))) | (dm << (p->pin * 3));
	}

	return 0;
}
