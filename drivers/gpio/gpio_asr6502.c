/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr GPIO driver for the ASR6502. One instance per GPIO port; the pin drive
 * mode and routing are applied through the HAL (bsp_io). Ports 0..3 have a
 * dedicated NVIC line and support edge-triggered pin interrupts with callbacks.
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
	bool     has_irq;
};

struct gpio_asr6502_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

#define ASR_REG(base, off) (*(volatile uint32_t *) ((base) + (off)))
#define ASR_DR(base)       ASR_REG((base), ASR_GPIO_DR)
#define ASR_PS(base)       ASR_REG((base), ASR_GPIO_PS)
#define ASR_PC(base)       ASR_REG((base), ASR_GPIO_PC)
#define ASR_INTR_CFG(base) ASR_REG((base), ASR_GPIO_INTR_CFG)
#define ASR_INTR(base)     ASR_REG((base), ASR_GPIO_INTR)

#define ASR_GPIO_PINS       8U
#define ASR_GPIO_PIN_MASK   0xFFU

/* INTR_CFG edge select, 2 bits per pin. */
#define ASR_EDGE_DISABLED   0U
#define ASR_EDGE_RISING     1U
#define ASR_EDGE_FALLING    2U
#define ASR_EDGE_BOTH       3U

static int gpio_asr6502_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_asr6502_config *cfg = dev->config;
	asr_gpio_drive_mode_t mode;
	unsigned int key;

	/* Pulls are selected through the drive mode and would be overridden by any
	 * output mode, so output-with-pull is not supported. */
	if (((flags & GPIO_OUTPUT) != 0U) &&
	    ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_INPUT) != 0U) {
		if (((flags & GPIO_PULL_UP) != 0U) && ((flags & GPIO_PULL_DOWN) != 0U)) {
			return -ENOTSUP;
		}
	}

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
	const struct gpio_asr6502_config *cfg = dev->config;
	uint32_t shift = (uint32_t) pin * 2U;
	uint32_t edge;
	unsigned int key;

	if (mode == GPIO_INT_MODE_DISABLED) {
		edge = ASR_EDGE_DISABLED;
	} else if (mode == GPIO_INT_MODE_EDGE) {
		if (!cfg->has_irq) {
			return -ENOTSUP;   /* this port has no NVIC line */
		}
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			edge = ASR_EDGE_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			edge = ASR_EDGE_RISING;
			break;
		case GPIO_INT_TRIG_BOTH:
			edge = ASR_EDGE_BOTH;
			break;
		default:
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;   /* the GPIO block has no level-sensitive mode */
	}

	key = irq_lock();
	ASR_INTR_CFG(cfg->base) =
		(ASR_INTR_CFG(cfg->base) & ~(0x3UL << shift)) | (edge << shift);
	ASR_INTR(cfg->base) = (1UL << pin);   /* clear any latched edge (W1C) */
	irq_unlock(key);

	return 0;
}

static int gpio_asr6502_manage_callback(const struct device *dev, struct gpio_callback *cb, bool set)
{
	struct gpio_asr6502_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_asr6502_port_get_direction(const struct device *dev, gpio_port_pins_t map,
					   gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_asr6502_config *cfg = dev->config;
	uint32_t pc = ASR_PC(cfg->base);
	gpio_port_pins_t in = 0U;
	gpio_port_pins_t out = 0U;

	map &= ASR_GPIO_PIN_MASK;

	for (uint8_t pin = 0; pin < ASR_GPIO_PINS; pin++) {
		if ((map & BIT(pin)) == 0U) {
			continue;
		}

		switch ((pc >> (pin * 3U)) & 0x7U) {
		case ASR_GPIO_DM_INPUT:
		case ASR_GPIO_DM_PULLUP:
		case ASR_GPIO_DM_PULLDOWN:
			in |= BIT(pin);
			break;
		case ASR_GPIO_DM_OD_DRIVES_LOW:
		case ASR_GPIO_DM_OD_DRIVES_HIGH:
		case ASR_GPIO_DM_STRONG:
			out |= BIT(pin);
			break;
		default:
			break;   /* analog / disconnected */
		}
	}

	if (inputs != NULL) {
		*inputs = in;
	}
	if (outputs != NULL) {
		*outputs = out;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static void gpio_asr6502_isr(const struct device *dev)
{
	const struct gpio_asr6502_config *cfg = dev->config;
	struct gpio_asr6502_data *data = dev->data;
	uint32_t status = ASR_INTR(cfg->base) & ASR_GPIO_PIN_MASK;

	if (status == 0U) {
		return;
	}

	ASR_INTR(cfg->base) = status;   /* clear serviced edges (W1C) */
	gpio_fire_callbacks(&data->callbacks, dev, status);
}

static DEVICE_API(gpio, gpio_asr6502_api) = {
	.pin_configure = gpio_asr6502_pin_configure,
	.port_get_raw = gpio_asr6502_port_get_raw,
	.port_set_masked_raw = gpio_asr6502_port_set_masked_raw,
	.port_set_bits_raw = gpio_asr6502_port_set_bits_raw,
	.port_clear_bits_raw = gpio_asr6502_port_clear_bits_raw,
	.port_toggle_bits = gpio_asr6502_port_toggle_bits,
	.pin_interrupt_configure = gpio_asr6502_pin_interrupt_configure,
	.manage_callback = gpio_asr6502_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_asr6502_port_get_direction,
#endif
};

/* Connect and enable the port's NVIC line (only for instances that have one). */
#define GPIO_ASR6502_IRQ(n)								\
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),				\
		    gpio_asr6502_isr, DEVICE_DT_INST_GET(n), 0);			\
	irq_enable(DT_INST_IRQN(n));

#define GPIO_ASR6502_INIT(n)								\
	static int gpio_asr6502_init_##n(const struct device *dev)			\
	{										\
		ARG_UNUSED(dev);							\
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 0), (GPIO_ASR6502_IRQ(n)))		\
		return 0;								\
	}										\
	static const struct gpio_asr6502_config gpio_asr6502_cfg_##n = {		\
		.common = {								\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),		\
		},									\
		.base = DT_INST_REG_ADDR(n),						\
		.port = (DT_INST_REG_ADDR(n) - ASR_GPIO_BASE) / ASR_GPIO_PRT_SIZE,	\
		.has_irq = DT_INST_IRQ_HAS_IDX(n, 0),					\
	};										\
	static struct gpio_asr6502_data gpio_asr6502_data_##n;				\
	DEVICE_DT_INST_DEFINE(n, gpio_asr6502_init_##n, NULL,				\
			      &gpio_asr6502_data_##n, &gpio_asr6502_cfg_##n,		\
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,			\
			      &gpio_asr6502_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ASR6502_INIT)
