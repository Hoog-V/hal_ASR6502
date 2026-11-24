/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr ADC driver for the ASR6502 SAR. Implements the ADC subsystem API:
 * single-ended and differential channels, multi-channel sequences, hardware
 * averaging (oversampling), selectable sample time / reference, and interrupt-
 * driven (optionally asynchronous) acquisition.
 */

#define DT_DRV_COMPAT asr_asr6502_sar

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "asr6502.h"

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(adc_asr6502, CONFIG_ADC_LOG_LEVEL);

#define ASR_SAR_NUM_CHANNELS   16U
#define ASR_SAR_RESOLUTION     12U

struct adc_asr6502_config {
	uint32_t base;
	uint16_t vref_mv;
	uint8_t  vref_sel;
	uint16_t sample_time;
	void (*irq_config)(void);
};

struct adc_asr6502_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;        /* active channels for the current sequence */
	uint8_t  resolution;
	/* Per-channel positive SARMUX pin, captured at channel setup. */
	uint8_t  chan_pin[ASR_SAR_NUM_CHANNELS];
};

#define SAR(base, off) (*(volatile uint32_t *) ((base) + (off)))

static int adc_asr6502_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *cfg)
{
	const struct adc_asr6502_config *dcfg = dev->config;
	struct adc_asr6502_data *data = dev->data;
	uint32_t chan_cfg;

	if (cfg->channel_id >= ASR_SAR_NUM_CHANNELS) {
		return -EINVAL;
	}

	if (cfg->gain != ADC_GAIN_1) {
		LOG_ERR("only ADC_GAIN_1 is supported");
		return -ENOTSUP;
	}

	/* The reference voltage is fixed by the controller (vref-sel / vref-mv). */
	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("only ADC_ACQ_TIME_DEFAULT is supported");
		return -ENOTSUP;
	}

	chan_cfg = ((uint32_t) cfg->input_positive << ASR_SAR_CHAN_CONFIG_PIN_ADDR_Pos) &
		   ASR_SAR_CHAN_CONFIG_PIN_ADDR_Msk;

	if (cfg->differential) {
		/* Differential uses the SARMUX adjacent-pin pair (pos even, neg = pos+1). */
		chan_cfg |= ASR_SAR_CHAN_CONFIG_DIFF_EN_Msk;
	}

	SAR(dcfg->base, ASR_SAR_CHAN_CONFIG + cfg->channel_id * 4UL) = chan_cfg;
	data->chan_pin[cfg->channel_id] = cfg->input_positive & 0x7U;

	return 0;
}

static int adc_asr6502_validate_sequence(const struct adc_sequence *seq)
{
	if (seq->resolution == 0U || seq->resolution > ASR_SAR_RESOLUTION) {
		LOG_ERR("unsupported resolution %u", seq->resolution);
		return -EINVAL;
	}

	if (seq->channels == 0U ||
	    (seq->channels >> ASR_SAR_NUM_CHANNELS) != 0U) {
		return -EINVAL;
	}

	return 0;
}

static int adc_asr6502_start_read(const struct device *dev, const struct adc_sequence *seq)
{
	const struct adc_asr6502_config *dcfg = dev->config;
	struct adc_asr6502_data *data = dev->data;
	uint32_t sample_ctrl = 0U;
	size_t need;
	int err;

	err = adc_asr6502_validate_sequence(seq);
	if (err < 0) {
		return err;
	}

	/* Buffer must hold one 16-bit sample per active channel (x repeats). */
	need = (size_t) POPCOUNT(seq->channels) * sizeof(uint16_t);
	if (seq->options != NULL) {
		need *= (1U + seq->options->extra_samplings);
	}
	if (seq->buffer_size < need) {
		return -ENOMEM;
	}

	/* Hardware averaging requested through oversampling (samples = 2^oversampling). */
	if (seq->oversampling != 0U) {
		if (seq->oversampling > 8U) {
			return -ENOTSUP;
		}
		sample_ctrl |= (((uint32_t) seq->oversampling - 1U)
				 << ASR_SAR_SAMPLE_CTRL_AVG_CNT_Pos) &
			       ASR_SAR_SAMPLE_CTRL_AVG_CNT_Msk;
		sample_ctrl |= ASR_SAR_SAMPLE_CTRL_AVG_SHIFT_Msk;

		for (uint32_t ch = seq->channels; ch != 0U;) {
			uint8_t i = (uint8_t) (find_lsb_set(ch) - 1);

			ch &= ~BIT(i);
			SAR(dcfg->base, ASR_SAR_CHAN_CONFIG + i * 4UL) |=
				ASR_SAR_CHAN_CONFIG_AVG_EN_Msk;
		}
	}

	SAR(dcfg->base, ASR_SAR_SAMPLE_CTRL) = sample_ctrl;

	data->resolution = seq->resolution;
	data->buffer = seq->buffer;

	adc_context_start_read(&data->ctx, seq);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_asr6502_read_async(const struct device *dev, const struct adc_sequence *seq,
				  struct k_poll_signal *async)
{
	struct adc_asr6502_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async != NULL, async);
	err = adc_asr6502_start_read(dev, seq);
	adc_context_release(&data->ctx, err);

	return err;
}

static int adc_asr6502_read(const struct device *dev, const struct adc_sequence *seq)
{
	return adc_asr6502_read_async(dev, seq, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_asr6502_data *data = CONTAINER_OF(ctx, struct adc_asr6502_data, ctx);
	const struct adc_asr6502_config *dcfg = data->dev->config;
	uint32_t mux = ASR_SAR_MUX_VSSA_VMINUS_Msk;

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	/* Enable the selected channels and connect their SARMUX inputs. */
	SAR(dcfg->base, ASR_SAR_CHAN_EN) = data->channels;
	for (uint32_t ch = data->channels; ch != 0U;) {
		uint8_t i = (uint8_t) (find_lsb_set(ch) - 1);

		ch &= ~BIT(i);
		mux |= BIT(data->chan_pin[i]);
	}
	SAR(dcfg->base, ASR_SAR_MUX_SWITCH0) = mux;

	/* Arm the end-of-scan interrupt and trigger the conversion. */
	SAR(dcfg->base, ASR_SAR_INTR) = ASR_SAR_INTR_EOS_Msk;
	SAR(dcfg->base, ASR_SAR_INTR_MASK) = ASR_SAR_INTR_EOS_Msk;
	SAR(dcfg->base, ASR_SAR_START_CTRL) = 0x1UL;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_asr6502_data *data = CONTAINER_OF(ctx, struct adc_asr6502_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_asr6502_isr(const struct device *dev)
{
	const struct adc_asr6502_config *dcfg = dev->config;
	struct adc_asr6502_data *data = dev->data;

	if ((SAR(dcfg->base, ASR_SAR_INTR) & ASR_SAR_INTR_EOS_Msk) == 0U) {
		return;
	}

	SAR(dcfg->base, ASR_SAR_INTR) = ASR_SAR_INTR_EOS_Msk;
	SAR(dcfg->base, ASR_SAR_INTR_MASK) = 0U;

	for (uint32_t ch = data->channels; ch != 0U;) {
		uint8_t i = (uint8_t) (find_lsb_set(ch) - 1);
		uint16_t raw = (uint16_t) SAR(dcfg->base, ASR_SAR_CHAN_RESULT + i * 4UL);

		ch &= ~BIT(i);
		/* SAR converts at 12-bit; narrow to the requested resolution. */
		*data->buffer++ = raw >> (ASR_SAR_RESOLUTION - data->resolution);
	}

	adc_context_on_sampling_done(&data->ctx, dev);
}

static int adc_asr6502_init(const struct device *dev)
{
	const struct adc_asr6502_config *dcfg = dev->config;
	struct adc_asr6502_data *data = dev->data;

	data->dev = dev;

	SAR(dcfg->base, ASR_SAR_SAMPLE_TIME01) =
		(uint32_t) dcfg->sample_time & ASR_SAR_SAMPLE_TIME0_Msk;
	SAR(dcfg->base, ASR_SAR_CTRL) =
		ASR_SAR_CTRL_ENABLED_Msk |
		(((uint32_t) dcfg->vref_sel << ASR_SAR_CTRL_VREF_SEL_Pos) &
		 ASR_SAR_CTRL_VREF_SEL_Msk);

	dcfg->irq_config();
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_asr6502_api) = {
	.channel_setup = adc_asr6502_channel_setup,
	.read = adc_asr6502_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_asr6502_read_async,
#endif
	.ref_internal = DT_INST_PROP(0, vref_mv),
};

#define ADC_ASR6502_INIT(n)								\
	static void adc_asr6502_irq_config_##n(void)					\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    adc_asr6502_isr, DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
	static const struct adc_asr6502_config adc_asr6502_cfg_##n = {			\
		.base = DT_INST_REG_ADDR(n),						\
		.vref_mv = DT_INST_PROP(n, vref_mv),					\
		.vref_sel = DT_INST_PROP(n, vref_sel),					\
		.sample_time = DT_INST_PROP(n, sample_time),				\
		.irq_config = adc_asr6502_irq_config_##n,				\
	};										\
	static struct adc_asr6502_data adc_asr6502_data_##n = {				\
		ADC_CONTEXT_INIT_TIMER(adc_asr6502_data_##n, ctx),			\
		ADC_CONTEXT_INIT_LOCK(adc_asr6502_data_##n, ctx),			\
		ADC_CONTEXT_INIT_SYNC(adc_asr6502_data_##n, ctx),			\
	};										\
	DEVICE_DT_INST_DEFINE(n, adc_asr6502_init, NULL,				\
			      &adc_asr6502_data_##n, &adc_asr6502_cfg_##n,		\
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,			\
			      &adc_asr6502_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ASR6502_INIT)
