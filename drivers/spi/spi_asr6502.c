/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Polling SPI master driver for the ASR6502 SCB. Full-duplex, 8-bit words, MSB
 * first. Chip select is handled by the SPI subsystem via cs-gpios. Used as the
 * bus for the on-die SX126x radio (SCB0).
 */

#define DT_DRV_COMPAT asr_asr6502_spi

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_asr6502, CONFIG_SPI_LOG_LEVEL);

#include "spi_context.h"
#include "asr6502.h"
#include "bsp_clocks.h"

#define REG(addr) (*(volatile uint32_t *)(addr))

/* SPI SCLK = HFCLK / peridiv / OVS. Fixed OVS keeps the math simple. */
#define SPI_OVS 8U

/* Upper bound on one byte's FIFO round-trip, far above the ~1-80 us a byte
 * takes on the wire: a real transfer never trips it, a wedged SCB does. */
#define SPI_XFER_TIMEOUT_US 10000U

struct spi_asr6502_config {
	uint32_t base;
	const struct pinctrl_dev_config *pcfg;
};

struct spi_asr6502_data {
	struct spi_context ctx;
};

static void spi_asr6502_clock_init(uint32_t instance, uint32_t freq_hz)
{
	uint32_t hfclk = asr_clock_hfclk_get();
	uint32_t div;

	if (hfclk == 0U) {
		hfclk = 48000000U;
	}

	div = hfclk / (SPI_OVS * freq_hz);
	if (div < 1U) {
		div = 1U;
	}

	REG(ASR_PERI_BASE + ASR_PERI_DIV_16_CTL(instance)) =
		(div - 1U) << ASR_PERI_DIV_16_INT_Pos;
	REG(ASR_PERI_BASE + ASR_PERI_PCLK_CTL(instance)) =
		(ASR_PERI_DIV_TYPE_16 << ASR_PERI_PCLK_SEL_TYPE_Pos) | instance;
	REG(ASR_PERI_BASE + ASR_PERI_DIV_CMD) =
		ASR_PERI_DIV_CMD_ENABLE_Msk | ASR_PERI_DIV_CMD_PA_NONE |
		(ASR_PERI_DIV_TYPE_16 << ASR_PERI_DIV_CMD_SEL_TYPE_Pos) | instance;
}

static int spi_asr6502_configure(const struct spi_asr6502_config *cfg,
				 struct spi_asr6502_data *data,
				 const struct spi_config *spi_cfg)
{
	uint32_t base = cfg->base;
	uint32_t spi_ctrl;

	if (spi_context_configured(&data->ctx, spi_cfg)) {
		return 0;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		return -ENOTSUP;
	}
	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		return -ENOTSUP;
	}
	if ((spi_cfg->operation & SPI_TRANSFER_LSB) != 0U) {
		return -ENOTSUP;   /* hardware is configured MSB first */
	}

	spi_asr6502_clock_init(ASR_SCB_INSTANCE(base), spi_cfg->frequency);

	REG(base + ASR_SCB_CTRL) &= ~ASR_SCB_CTRL_ENABLED_Msk;

	spi_ctrl = ASR_SCB_SPI_CTRL_MASTER_Msk | ASR_SCB_SPI_CTRL_CONTINUOUS_Msk;
	if ((spi_cfg->operation & SPI_MODE_CPHA) != 0U) {
		spi_ctrl |= ASR_SCB_SPI_CTRL_CPHA_Msk;
	}
	if ((spi_cfg->operation & SPI_MODE_CPOL) != 0U) {
		spi_ctrl |= ASR_SCB_SPI_CTRL_CPOL_Msk;
	}
	REG(base + ASR_SCB_SPI_CTRL) = spi_ctrl;

	REG(base + ASR_SCB_TX_CTRL) = ASR_SCB_CTRL_MSB_FIRST_Msk | 7U; /* 8 bits */
	REG(base + ASR_SCB_RX_CTRL) = ASR_SCB_CTRL_MSB_FIRST_Msk | 7U;
	REG(base + ASR_SCB_CTRL) = ASR_SCB_CTRL_MODE_SPI | (SPI_OVS - 1U);
	REG(base + ASR_SCB_CTRL) |= ASR_SCB_CTRL_ENABLED_Msk;

	data->ctx.config = spi_cfg;

	return 0;
}

static int spi_asr6502_transfer_byte(uint32_t base, uint8_t out, uint8_t *in)
{
	uint32_t deadline = k_cycle_get_32() + k_us_to_cyc_ceil32(SPI_XFER_TIMEOUT_US);

	while ((REG(base + ASR_SCB_TX_FIFO_STATUS) & ASR_SCB_FIFO_USED_Msk) >=
	       ASR_SCB_FIFO_DEPTH) {
		if ((int32_t)(k_cycle_get_32() - deadline) >= 0) {
			return -ETIMEDOUT;
		}
	}
	REG(base + ASR_SCB_TX_FIFO_WR) = out;

	while ((REG(base + ASR_SCB_RX_FIFO_STATUS) & ASR_SCB_FIFO_USED_Msk) == 0U) {
		if ((int32_t)(k_cycle_get_32() - deadline) >= 0) {
			return -ETIMEDOUT;
		}
	}
	*in = (uint8_t)REG(base + ASR_SCB_RX_FIFO_RD);
	return 0;
}

static int spi_asr6502_transceive(const struct device *dev,
				  const struct spi_config *spi_cfg,
				  const struct spi_buf_set *tx_bufs,
				  const struct spi_buf_set *rx_bufs)
{
	const struct spi_asr6502_config *cfg = dev->config;
	struct spi_asr6502_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, false, NULL, NULL, spi_cfg);

	ret = spi_asr6502_configure(cfg, data, spi_cfg);
	if (ret < 0) {
		goto done;
	}

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);
	spi_context_cs_control(ctx, true);

	while (spi_context_tx_buf_on(ctx) || spi_context_rx_buf_on(ctx)) {
		uint8_t out = spi_context_tx_buf_on(ctx) ? *ctx->tx_buf : 0U;
		uint8_t in;

		ret = spi_asr6502_transfer_byte(cfg->base, out, &in);
		if (ret < 0) {
			break;
		}

		if (spi_context_rx_buf_on(ctx)) {
			*ctx->rx_buf = in;
		}

		spi_context_update_tx(ctx, 1, 1);
		spi_context_update_rx(ctx, 1, 1);
	}

	spi_context_cs_control(ctx, false);

done:
	spi_context_release(ctx, ret);

	return ret;
}

static int spi_asr6502_release(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_asr6502_data *data = dev->data;

	ARG_UNUSED(spi_cfg);
	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(spi, spi_asr6502_api) = {
	.transceive = spi_asr6502_transceive,
	.release = spi_asr6502_release,
};

static int spi_asr6502_init(const struct device *dev)
{
	const struct spi_asr6502_config *cfg = dev->config;
	struct spi_asr6502_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		return ret;
	}

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define SPI_ASR6502_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	static struct spi_asr6502_data spi_asr6502_data_##n = {			\
		SPI_CONTEXT_INIT_LOCK(spi_asr6502_data_##n, ctx),		\
		SPI_CONTEXT_INIT_SYNC(spi_asr6502_data_##n, ctx),		\
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)		\
	};									\
	static const struct spi_asr6502_config spi_asr6502_cfg_##n = {		\
		.base = DT_INST_REG_ADDR(n),					\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	};									\
	SPI_DEVICE_DT_INST_DEFINE(n, spi_asr6502_init, NULL,			\
				  &spi_asr6502_data_##n, &spi_asr6502_cfg_##n,	\
				  POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,	\
				  &spi_asr6502_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_ASR6502_INIT)
