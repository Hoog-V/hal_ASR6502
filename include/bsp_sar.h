/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SAR ADC driver for the ASR6502: single-ended, firmware-triggered one-shot
 * conversions. A channel samples a SARMUX input pin against VSSA.
 */

#ifndef BSP_SAR_H
#define BSP_SAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Enable and configure the SAR.
 *
 * vref_sel     3-bit CTRL.VREF_SEL field selecting the conversion reference.
 *              The value-to-reference mapping is device-specific; take it from
 *              the ASR6502/PSoC 4 datasheet (it sets the full-scale voltage).
 * sample_time  Aperture in ADC clocks for the shared sample timer ST0
 *              (10-bit; a larger value suits higher source impedance).
 */
void asr_sar_init(uint8_t vref_sel, uint16_t sample_time);

/*
 * Configure a result channel (0..15) to read SARMUX input pin sarmux_pin
 * (0..7) single-ended against VSSA, and enable it.
 */
void asr_sar_channel_config(uint8_t chan, uint8_t sarmux_pin);

/*
 * Run one conversion on an enabled channel and return its raw result
 * (signed 16-bit; unsigned single-ended results occupy the low 12 bits).
 */
int16_t asr_sar_read(uint8_t chan);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SAR_H */
