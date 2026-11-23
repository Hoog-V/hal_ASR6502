/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPIO and HSIOM access for the ASR6502.
 */

#include "bsp_io.h"

/* 32-bit register at a GPIO port base plus offset. */
#define ASR_GPIO_REG(port, off)    (*(volatile uint32_t *) (ASR_GPIO_PRT_BASE(port) + (off)))

/* HSIOM PORT_SEL0 register for a port. */
#define ASR_HSIOM_REG(port)        (*(volatile uint32_t *) (ASR_HSIOM_PRT_BASE(port) + ASR_HSIOM_PORT_SEL0))

void asr_hsiom_set(uint32_t port, uint32_t pin, uint32_t sel)
{
    uint32_t shift = pin * 4U;                       /* 4 bits per pin */

    ASR_HSIOM_REG(port) = (ASR_HSIOM_REG(port) & ~(0xFUL << shift)) | ((sel & 0xFUL) << shift);
}

void asr_gpio_set_drive_mode(uint32_t port, uint32_t pin, asr_gpio_drive_mode_t mode)
{
    uint32_t shift = pin * 3U;                       /* 3 bits per pin in PC */

    ASR_GPIO_REG(port, ASR_GPIO_PC) =
        (ASR_GPIO_REG(port, ASR_GPIO_PC) & ~(0x7UL << shift)) | (((uint32_t) mode & 0x7UL) << shift);
}

void asr_gpio_write(uint32_t port, uint32_t pin, uint32_t value)
{
    /* Read-modify-write of the output data register. */
    if (value != 0U) {
        ASR_GPIO_REG(port, ASR_GPIO_DR) |= (1UL << pin);
    } else {
        ASR_GPIO_REG(port, ASR_GPIO_DR) &= ~(1UL << pin);
    }
}

void asr_gpio_toggle(uint32_t port, uint32_t pin)
{
    ASR_GPIO_REG(port, ASR_GPIO_DR) ^= (1UL << pin);
}

uint32_t asr_gpio_read(uint32_t port, uint32_t pin)
{
    return (ASR_GPIO_REG(port, ASR_GPIO_PS) >> pin) & 1UL;
}

void asr_gpio_configure(uint32_t port, uint32_t pin, asr_gpio_drive_mode_t mode)
{
    asr_hsiom_set(port, pin, ASR_HSIOM_SEL_GPIO);
    asr_gpio_set_drive_mode(port, pin, mode);
}
