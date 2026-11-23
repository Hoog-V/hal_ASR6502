/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPIO and pin-routing (HSIOM) access for the ASR6502. Pins are addressed by
 * port (0..ASR_GPIO_PORT_COUNT-1) and pin (0..7).
 */

#ifndef BSP_IO_H
#define BSP_IO_H

#include <stdint.h>
#include "asr6502.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Pin drive mode (GPIO_PRT.PC, 3 bits per pin). */
typedef enum
{
    ASR_GPIO_DM_ANALOG         = 0, /* High-Z, input buffer off (analog) */
    ASR_GPIO_DM_INPUT          = 1, /* High-Z digital input */
    ASR_GPIO_DM_PULLUP         = 2, /* Resistive pull-up */
    ASR_GPIO_DM_PULLDOWN       = 3, /* Resistive pull-down */
    ASR_GPIO_DM_OD_DRIVES_LOW  = 4, /* Open drain, drives low */
    ASR_GPIO_DM_OD_DRIVES_HIGH = 5, /* Open drain, drives high */
    ASR_GPIO_DM_STRONG         = 6, /* Strong drive (push-pull output) */
    ASR_GPIO_DM_PULLUP_DOWN    = 7, /* Resistive pull-up and pull-down */
} asr_gpio_drive_mode_t;

/* HSIOM routing select (HSIOM_PRT.PORT_SEL0, 4 bits per pin). 0 = plain GPIO;
 * peripheral routings are device-specific, pass the raw value from the manual. */
typedef enum
{
    ASR_HSIOM_SEL_GPIO = 0,
} asr_hsiom_sel_t;

/* Route a pin to a function (HSIOM). Use ASR_HSIOM_SEL_GPIO for software I/O. */
void asr_hsiom_set(uint32_t port, uint32_t pin, uint32_t sel);

/* Set a pin's drive mode. */
void asr_gpio_set_drive_mode(uint32_t port, uint32_t pin, asr_gpio_drive_mode_t mode);

/* Drive a pin's output latch (0 or non-zero). */
void asr_gpio_write(uint32_t port, uint32_t pin, uint32_t value);

/* Toggle a pin's output latch. */
void asr_gpio_toggle(uint32_t port, uint32_t pin);

/* Read a pin's live input state (0 or 1). */
uint32_t asr_gpio_read(uint32_t port, uint32_t pin);

/* Route to GPIO and set the drive mode in one call. */
void asr_gpio_configure(uint32_t port, uint32_t pin, asr_gpio_drive_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* BSP_IO_H */
