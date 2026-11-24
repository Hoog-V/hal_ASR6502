/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DT_BINDINGS_PINCTRL_ASR6502_H_
#define DT_BINDINGS_PINCTRL_ASR6502_H_

/*
 * Pack a pin's routing into one cell: GPIO port, pin, and the HSIOM select
 * value that connects it to a peripheral signal (from the device datasheet).
 */
#define ASR_PINMUX(port, pin, hsiom) \
	((((port) & 0x7) << 8) | (((pin) & 0x7) << 4) | ((hsiom) & 0xF))

#define ASR_PINMUX_PORT(m)	(((m) >> 8) & 0x7)
#define ASR_PINMUX_PIN(m)	(((m) >> 4) & 0x7)
#define ASR_PINMUX_HSIOM(m)	((m) & 0xF)

#endif /* DT_BINDINGS_PINCTRL_ASR6502_H_ */
