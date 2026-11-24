/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr counter driver backed by the ASR6502 watchdog timer. The WDT is a
 * free-running 16-bit counter clocked by the ILO (~40 kHz) that keeps running in
 * deep sleep, so its match interrupt (SRSS, IRQ 6) can wake the SoC on a timer.
 *
 * The driver writes the WDT disable key, which turns off the reset function
 * only; the counter and match interrupt keep working. Enabling this counter
 * therefore repurposes the watchdog -- it no longer resets on lockup.
 */

#define DT_DRV_COMPAT asr_asr6502_counter

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>

#include "asr6502.h"
#include "bsp_clocks.h"

#define WDT_TOP_VALUE   0xFFFFU

struct counter_asr6502_config {
	struct counter_config_info info;
	uint32_t base;
	void (*irq_config)(void);
};

struct counter_asr6502_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

#define SRSSLT(base, off) (*(volatile uint32_t *) ((base) + (off)))

static int counter_asr6502_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_asr6502_config *cfg = dev->config;

	*ticks = SRSSLT(cfg->base, ASR_SRSSLT_WDT_COUNTER) & WDT_TOP_VALUE;

	return 0;
}

static int counter_asr6502_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_asr6502_config *cfg = dev->config;
	struct counter_asr6502_data *data = dev->data;
	uint32_t match;
	unsigned int key;

	if (chan_id != 0U) {
		return -ENOTSUP;
	}
	if (alarm_cfg->ticks > WDT_TOP_VALUE) {
		return -EINVAL;
	}

	key = irq_lock();

	if (data->callback != NULL) {
		irq_unlock(key);
		return -EBUSY;
	}

	data->callback = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U) {
		match = alarm_cfg->ticks;
	} else {
		match = (SRSSLT(cfg->base, ASR_SRSSLT_WDT_COUNTER) + alarm_cfg->ticks) & WDT_TOP_VALUE;
	}

	SRSSLT(cfg->base, ASR_SRSSLT_WDT_MATCH) = match;
	SRSSLT(cfg->base, ASR_SRSSLT_SRSS_INTR) = ASR_SRSS_INTR_WDT_MATCH_Msk;   /* clear */
	SRSSLT(cfg->base, ASR_SRSSLT_SRSS_INTR_MASK) |= ASR_SRSS_INTR_WDT_MATCH_Msk;

	irq_unlock(key);

	return 0;
}

static int counter_asr6502_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_asr6502_config *cfg = dev->config;
	struct counter_asr6502_data *data = dev->data;
	unsigned int key = irq_lock();

	if (chan_id == 0U) {
		SRSSLT(cfg->base, ASR_SRSSLT_SRSS_INTR_MASK) &= ~ASR_SRSS_INTR_WDT_MATCH_Msk;
		data->callback = NULL;
	}

	irq_unlock(key);

	return 0;
}

static int counter_asr6502_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *top_cfg)
{
	ARG_UNUSED(dev);

	/* The WDT counter is a fixed 16-bit free-running counter. */
	if (top_cfg->ticks != WDT_TOP_VALUE) {
		return -ENOTSUP;
	}

	return 0;
}

static uint32_t counter_asr6502_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);

	return WDT_TOP_VALUE;
}

static uint32_t counter_asr6502_get_pending_int(const struct device *dev)
{
	const struct counter_asr6502_config *cfg = dev->config;

	return (SRSSLT(cfg->base, ASR_SRSSLT_SRSS_INTR) & ASR_SRSS_INTR_WDT_MATCH_Msk) ? 1U : 0U;
}

static int counter_asr6502_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* The WDT counter free-runs once the ILO is enabled (done at init). */
	return 0;
}

static int counter_asr6502_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* The free-running WDT counter cannot be halted. */
	return -ENOTSUP;
}

static void counter_asr6502_isr(const struct device *dev)
{
	const struct counter_asr6502_config *cfg = dev->config;
	struct counter_asr6502_data *data = dev->data;
	counter_alarm_callback_t cb;
	uint32_t now;

	if ((SRSSLT(cfg->base, ASR_SRSSLT_SRSS_INTR) & ASR_SRSS_INTR_WDT_MATCH_Msk) == 0U) {
		return;
	}

	/* Clear and disarm (one-shot alarm). */
	SRSSLT(cfg->base, ASR_SRSSLT_SRSS_INTR) = ASR_SRSS_INTR_WDT_MATCH_Msk;
	SRSSLT(cfg->base, ASR_SRSSLT_SRSS_INTR_MASK) &= ~ASR_SRSS_INTR_WDT_MATCH_Msk;

	cb = data->callback;
	data->callback = NULL;

	if (cb != NULL) {
		now = SRSSLT(cfg->base, ASR_SRSSLT_WDT_COUNTER) & WDT_TOP_VALUE;
		cb(dev, 0U, now, data->user_data);
	}
}

static int counter_asr6502_init(const struct device *dev)
{
	const struct counter_asr6502_config *cfg = dev->config;

	/* The ILO clocks the WDT counter and must run for it to tick. */
	asr_clock_ilo_enable(true);

	/* Disable the watchdog reset; the counter and match interrupt stay active. */
	SRSSLT(cfg->base, ASR_SRSSLT_WDT_DISABLE_KEY) = ASR_WDT_DISABLE_KEY;
	SRSSLT(cfg->base, ASR_SRSSLT_SRSS_INTR_MASK) &= ~ASR_SRSS_INTR_WDT_MATCH_Msk;

	cfg->irq_config();

	return 0;
}

static DEVICE_API(counter, counter_asr6502_api) = {
	.start = counter_asr6502_start,
	.stop = counter_asr6502_stop,
	.get_value = counter_asr6502_get_value,
	.set_alarm = counter_asr6502_set_alarm,
	.cancel_alarm = counter_asr6502_cancel_alarm,
	.set_top_value = counter_asr6502_set_top_value,
	.get_pending_int = counter_asr6502_get_pending_int,
	.get_top_value = counter_asr6502_get_top_value,
};

#define COUNTER_ASR6502_INIT(n)								\
	static void counter_asr6502_irq_config_##n(void)				\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    counter_asr6502_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
	static struct counter_asr6502_data counter_asr6502_data_##n;			\
	static const struct counter_asr6502_config counter_asr6502_cfg_##n = {		\
		.info = {								\
			.max_top_value = WDT_TOP_VALUE,					\
			.freq = ASR_CLK_ILO_FREQ_HZ,					\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,				\
			.channels = 1U,							\
		},									\
		.base = DT_INST_REG_ADDR(n),						\
		.irq_config = counter_asr6502_irq_config_##n,				\
	};										\
	DEVICE_DT_INST_DEFINE(n, counter_asr6502_init, NULL,				\
			      &counter_asr6502_data_##n, &counter_asr6502_cfg_##n,	\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
			      &counter_asr6502_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_ASR6502_INIT)
