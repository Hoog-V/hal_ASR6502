/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Polling UART driver for the ASR6502 SCB (Serial Communication Block) in UART
 * mode, 8N1. Configures the SCB, its baud clock (a PERI 16-bit divider off
 * HFCLK assigned to the block's PCLK), and routes the TX/RX pins via HSIOM.
 *
 * ponytail: polling only (poll_in/poll_out). Interrupt-driven/async API and
 * runtime reconfigure are not implemented; add if a shell or DMA path needs it.
 */

#define DT_DRV_COMPAT asr_asr6502_uart

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>

#include "asr6502.h"
#include "bsp_clocks.h"

#define REG(addr) (*(volatile uint32_t *)(addr))

struct uart_asr6502_config {
	uint32_t base;
	uint32_t current_speed;
	const struct pinctrl_dev_config *pcfg;
};

static void uart_asr6502_clock_init(uint32_t instance, uint32_t baud)
{
	uint32_t hfclk = asr_clock_hfclk_get();
	uint32_t ovs = 16U;
	uint32_t div;

	if (hfclk == 0U) {
		hfclk = 48000000U;   /* HFCLK source unknown to the HAL; assume IMO */
	}

	/* scb_clk = HFCLK / div; baud = scb_clk / ovs. Divider divides by div. */
	div = (hfclk + (baud * ovs) / 2U) / (baud * ovs);
	if (div < 1U) {
		div = 1U;
	}

	/* Program the 16-bit divider (value = div - 1), assign it to the SCB's
	 * PCLK, then enable it.
	 */
	REG(ASR_PERI_BASE + ASR_PERI_DIV_16_CTL(instance)) =
		(div - 1U) << ASR_PERI_DIV_16_INT_Pos;
	REG(ASR_PERI_BASE + ASR_PERI_PCLK_CTL(instance)) =
		(ASR_PERI_DIV_TYPE_16 << ASR_PERI_PCLK_SEL_TYPE_Pos) | instance;
	REG(ASR_PERI_BASE + ASR_PERI_DIV_CMD) =
		ASR_PERI_DIV_CMD_ENABLE_Msk | ASR_PERI_DIV_CMD_PA_NONE |
		(ASR_PERI_DIV_TYPE_16 << ASR_PERI_DIV_CMD_SEL_TYPE_Pos) | instance;
}

static int uart_asr6502_init(const struct device *dev)
{
	const struct uart_asr6502_config *cfg = dev->config;
	uint32_t base = cfg->base;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	uart_asr6502_clock_init(ASR_SCB_INSTANCE(base), cfg->current_speed);

	/* 8N1 standard UART. */
	REG(base + ASR_SCB_CTRL) &= ~ASR_SCB_CTRL_ENABLED_Msk;
	REG(base + ASR_SCB_UART_CTRL) = 0U;                       /* MODE = standard */
	REG(base + ASR_SCB_UART_TX_CTRL) = 1U;                    /* 1 stop bit */
	REG(base + ASR_SCB_UART_RX_CTRL) = 1U;                    /* 1 stop bit */
	REG(base + ASR_SCB_TX_CTRL) = 7U;                         /* 8 data bits */
	REG(base + ASR_SCB_RX_CTRL) = 7U;                         /* 8 data bits */
	REG(base + ASR_SCB_CTRL) = ASR_SCB_CTRL_MODE_UART | (16U - 1U); /* OVS = 16 */
	REG(base + ASR_SCB_CTRL) |= ASR_SCB_CTRL_ENABLED_Msk;

	return 0;
}

static int uart_asr6502_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_asr6502_config *cfg = dev->config;

	if ((REG(cfg->base + ASR_SCB_RX_FIFO_STATUS) & ASR_SCB_FIFO_USED_Msk) == 0U) {
		return -1;
	}

	*c = (unsigned char)REG(cfg->base + ASR_SCB_RX_FIFO_RD);

	return 0;
}

static void uart_asr6502_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_asr6502_config *cfg = dev->config;

	while ((REG(cfg->base + ASR_SCB_TX_FIFO_STATUS) & ASR_SCB_FIFO_USED_Msk) >=
	       ASR_SCB_FIFO_DEPTH) {
		/* wait for room */
	}

	REG(cfg->base + ASR_SCB_TX_FIFO_WR) = c;
}

static DEVICE_API(uart, uart_asr6502_api) = {
	.poll_in = uart_asr6502_poll_in,
	.poll_out = uart_asr6502_poll_out,
};

#define UART_ASR6502_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	static const struct uart_asr6502_config uart_asr6502_cfg_##n = {	\
		.base = DT_INST_REG_ADDR(n),					\
		.current_speed = DT_INST_PROP(n, current_speed),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
	};									\
	DEVICE_DT_INST_DEFINE(n, uart_asr6502_init, NULL, NULL,			\
			      &uart_asr6502_cfg_##n, PRE_KERNEL_1,		\
			      CONFIG_SERIAL_INIT_PRIORITY,			\
			      &uart_asr6502_api);

DT_INST_FOREACH_STATUS_OKAY(UART_ASR6502_INIT)
