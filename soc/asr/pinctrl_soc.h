/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ASR_PINCTRL_H_
#define ZEPHYR_SOC_ASR_PINCTRL_H_

#include <zephyr/types.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/dt-bindings/pinctrl/asr6502-pinctrl.h>

/* Drive-mode intent resolved by the pinctrl driver. */
#define ASR_PINCFG_INPUT	BIT(0)
#define ASR_PINCFG_PULLUP	BIT(1)
#define ASR_PINCFG_PULLDOWN	BIT(2)
#define ASR_PINCFG_OPENDRAIN	BIT(3)

typedef struct {
	uint8_t port;
	uint8_t pin;
	uint8_t hsiom;
	uint8_t flags;
} pinctrl_soc_pin_t;

#define Z_PINCTRL_ASR_PIN(node_id)							\
	{										\
		.port  = ASR_PINMUX_PORT(DT_PROP_BY_IDX(node_id, pinmux, 0)),		\
		.pin   = ASR_PINMUX_PIN(DT_PROP_BY_IDX(node_id, pinmux, 0)),		\
		.hsiom = ASR_PINMUX_HSIOM(DT_PROP_BY_IDX(node_id, pinmux, 0)),		\
		.flags = (DT_PROP(node_id, input_enable)     ? ASR_PINCFG_INPUT : 0) |	\
			 (DT_PROP(node_id, bias_pull_up)     ? ASR_PINCFG_PULLUP : 0) |	\
			 (DT_PROP(node_id, bias_pull_down)   ? ASR_PINCFG_PULLDOWN : 0) |\
			 (DT_PROP(node_id, drive_open_drain) ? ASR_PINCFG_OPENDRAIN : 0),\
	},

#define Z_PINCTRL_STATE_PIN_INIT(node_id, prop, idx)					\
	Z_PINCTRL_ASR_PIN(DT_PROP_BY_IDX(node_id, prop, idx))

#define Z_PINCTRL_STATE_PINS_INIT(node_id, prop)					\
	{ DT_FOREACH_PROP_ELEM(node_id, prop, Z_PINCTRL_STATE_PIN_INIT) }

#endif /* ZEPHYR_SOC_ASR_PINCTRL_H_ */
