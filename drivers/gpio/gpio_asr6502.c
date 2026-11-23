/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr GPIO driver for the ASR6502. One instance per GPIO port; the pin drive
 * mode and routing are applied through the HAL (bsp_io). Pin interrupts are not
 * yet implemented.
 */

#define DT_DRV_COMPAT asr_asr6502_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

#include "asr6502.h"
#include "bsp_io.h"

struct gpio_asr6502_config {
	struct gpio_driver_config common;
	uint32_t base;   /* GPIO port base address */
	uint8_t  port;   /* port index 0..7 */
};

struct gpio_asr6502_data {
	struct gpio_driver_data common;
};

#define ASR_DR(base) (*(volatile uint32_t *) ((base) + ASR_GPIO_DR))
#define ASR_PS(base) (*(volatile uint32_t *) ((base) + ASR_GPIO_PS))

static int gpio_asr6502_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_asr6502_config *cfg = dev->config;
	asr_gpio_drive_mode_t mode;
	unsigned int key;

	/* Validation before any register access. */

	/* The pull resistors are selected through the drive mode on this SoC and
	 * their setting would be overridden by any output mode; combinations of
	 * output and pull are therefore not supported. */
	if (((flags & GPIO_OUTPUT) != 0U) &&
	    ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_INPUT) != 0U) {
		/* Only one pull direction at a time. */
		if (((flags & GPIO_PULL_UP) != 0U) && ((flags & GPIO_PULL_DOWN) != 0U)) {
			return -ENOTSUP;
		}
	}

	/* Register updates that must not race with ISR-driven port writes or
	 * concurrent pin configuration (DRAM/shared registers are read-modify-
	 * written here). */
	key = irq_lock();

	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_SINGLE_ENDED) != 0U) {
			mode = (flags & GPIO_LINE_OPEN_DRAIN) ? ASR_GPIO_DM_OD_DRIVES_LOW
							      : ASR_GPIO_DM_OD_DRIVES_HIGH;
		} else {
			mode = ASR_GPIO_DM_STRONG;
		}

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			ASR_DR(cfg->base) |= (1UL << pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			ASR_DR(cfg->base) &= ~(1UL << pin);
		}
	} else if ((flags & GPIO_INPUT) != 0U) {
		if ((flags & GPIO_PULL_UP) != 0U) {
			mode = ASR_GPIO_DM_PULLUP;
			/* A resistive pull-up is enabled by driving the data latch high. */
			ASR_DR(cfg->base) |= (1UL << pin);
		} else if ((flags & GPIO_PULL_DOWN) != 0U) {
			mode = ASR_GPIO_DM_PULLDOWN;
			ASR_DR(cfg->base) &= ~(1UL << pin);
		} else {
			mode = ASR_GPIO_DM_INPUT;
		}
	} else {
		mode = ASR_GPIO_DM_ANALOG;   /* disconnected */
	}

	asr_hsiom_set(cfg->port, pin, ASR_HSIOM_SEL_GPIO);
	asr_gpio_set_drive_mode(cfg->port, pin, mode);

	irq_unlock(key);

	return 0;
}

static int gpio_asr6502_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_asr6502_config *cfg = dev->config;

	*value = ASR_PS(cfg->base);

	return 0;
}

static int gpio_asr6502_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_asr6502_config *cfg = dev->config;
	unsigned int key = irq_lock();

	ASR_DR(cfg->base) = (ASR_DR(cfg->base) & ~mask) | (value & mask);
	irq_unlock(key);

	return 0;
}

static int gpio_asr6502_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_asr6502_config *cfg = dev->config;
	unsigned int key = irq_lock();

	ASR_DR(cfg->base) |= pins;
	irq_unlock(key);

	return 0;
}

static int gpio_asr6502_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_asr6502_config *cfg = dev->config;
	unsigned int key = irq_lock();

	ASR_DR(cfg->base) &= ~pins;
	irq_unlock(key);

	return 0;
}

static int gpio_asr6502_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_asr6502_config *cfg = dev->config;
	unsigned int key = irq_lock();

	ASR_DR(cfg->base) ^= pins;
	irq_unlock(key);

	return 0;
}

static int gpio_asr6502_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);

	return -ENOTSUP;
}

static DEVICE_API(gpio, gpio_asr6502_api) = {
	.pin_configure = gpio_asr6502_pin_configure,
	.port_get_raw = gpio_asr6502_port_get_raw,
	.port_set_masked_raw = gpio_asr6502_port_set_masked_raw,
	.port_set_bits_raw = gpio_asr6502_port_set_bits_raw,
	.port_clear_bits_raw = gpio_asr6502_port_clear_bits_raw,
	.port_toggle_bits = gpio_asr6502_port_toggle_bits,
	.pin_interrupt_configure = gpio_asr6502_pin_interrupt_configure,
};

#define GPIO_ASR6502_INIT(n)								\
	static const struct gpio_asr6502_config gpio_asr6502_cfg_##n = {		\
		.common = {								\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),		\
		},									\
		.base = DT_INST_REG_ADDR(n),						\
		.port = (DT_INST_REG_ADDR(n) - ASR_GPIO_BASE) / ASR_GPIO_PRT_SIZE,	\
	};										\
	static struct gpio_asr6502_data gpio_asr6502_data_##n;				\
	DEVICE_DT_INST_DEFINE(n, NULL, NULL,						\
			      &gpio_asr6502_data_##n, &gpio_asr6502_cfg_##n,		\
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,			\
			      &gpio_asr6502_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ASR6502_INIT)
