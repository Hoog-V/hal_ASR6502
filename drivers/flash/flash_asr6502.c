/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal-flash driver for the ASR6502 (PSoC4100S+ / CY8C4147). Program/erase
 * go through the SROM system calls; the sequence (Load Latch -> clock backup/
 * config -> Write/Program Row -> clock restore, IRQs masked) is ported from
 * Infineon's mtb-pdl-cat2 cy_flash.c. 256-byte rows, two 64 KB macros, and the
 * erased state reads back as 0x00.
 */

#define DT_DRV_COMPAT asr_asr6502_flash

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <string.h>

#define SROM_SYSREQ  (*(volatile uint32_t *)0x40100004UL)
#define SROM_SYSARG  (*(volatile uint32_t *)0x40100008UL)
#define IMO_CONFIG   (*(volatile uint32_t *)0x40030030UL)
#define IMO_ENABLE   0x80000000UL

#define REQ_START    0x80000000UL
#define REQ_BUSY     0x90000000UL        /* SYSCALL_REQ | PRIVILEGED */
#define KEY1         0xB6UL
#define KEY2(x)      (0xD3UL + (uint32_t)(x))
#define OP_LOAD      0x04U
#define OP_WRITE_ROW 0x05U               /* erase + program */
#define OP_PROG_ROW  0x06U               /* program only */
#define OP_CLK_CFG   0x15U
#define OP_CLK_BKUP  0x16U
#define OP_CLK_REST  0x17U

#define FLASH_BASE    0x00000000UL
#define FLASH_SIZE    0x00020000UL
#define FLASH_MACROS  2U
#define FLASH_ROW     256U
#define FLASH_ERASE_VALUE 0x00U

#define MACRO_OF(addr) (((addr) - FLASH_BASE) / (FLASH_SIZE / FLASH_MACROS))

struct flash_asr6502_data {
	struct k_sem lock;
};

static uint32_t clk_backup[6];

static int srom_wait(void)
{
	uint32_t attempts = 2000000U;

	__asm__ volatile("dmb");
	while (((SROM_SYSREQ & REQ_BUSY) != 0U) && (attempts != 0U)) {
		attempts--;
	}
	if (attempts == 0U) {
		return -ETIMEDOUT;
	}
	return ((SROM_SYSARG & 0xF0000000UL) == 0xA0000000UL) ? 0 : -EIO;
}

/* Program one 256-byte row at rowAddr with the SROM. final_op is Write Row
 * (erase+program) or Program Row (program only).
 */
static int flash_row(uint32_t row_addr, const uint8_t *data, uint32_t final_op)
{
	uint32_t p[2 + FLASH_ROW / 4];
	uint32_t bp[2];
	unsigned int key;
	int ret;

	/* Load the page latch (opcode 0x04) with the row data. */
	p[0] = (MACRO_OF(row_addr) << 24) | (KEY2(OP_LOAD) << 8) | KEY1;
	p[1] = FLASH_ROW - 1U;
	memcpy(&p[2], data, FLASH_ROW);
	SROM_SYSARG = (uint32_t)p;
	SROM_SYSREQ = REQ_START | OP_LOAD;
	ret = srom_wait();
	if (ret != 0) {
		return ret;
	}

	key = irq_lock();

	/* Back up and configure the clock for the program/erase. */
	bp[0] = (KEY2(OP_CLK_BKUP) << 8) | KEY1;
	bp[1] = (uint32_t)&clk_backup[0];
	SROM_SYSARG = (uint32_t)bp;
	SROM_SYSREQ = REQ_START | OP_CLK_BKUP;
	ret = srom_wait();
	IMO_CONFIG |= IMO_ENABLE;             /* HW requires IMO re-enable here */
	if (ret == 0) {
		SROM_SYSARG = (KEY2(OP_CLK_CFG) << 8) | KEY1;
		SROM_SYSREQ = REQ_START | OP_CLK_CFG;
		ret = srom_wait();
	}

	/* Program/erase the row from the latch. */
	if (ret == 0) {
		p[0] = ((KEY2(final_op) << 8) | KEY1) | ((row_addr / FLASH_ROW) << 16);
		SROM_SYSARG = (uint32_t)p;
		SROM_SYSREQ = REQ_START | final_op;
		ret = srom_wait();
	}

	/* Always restore the clock. */
	bp[0] = (KEY2(OP_CLK_REST) << 8) | KEY1;
	bp[1] = (uint32_t)&clk_backup[0];
	SROM_SYSARG = (uint32_t)bp;
	SROM_SYSREQ = REQ_START | OP_CLK_REST;
	(void)srom_wait();

	irq_unlock(key);
	return ret;
}

static bool range_ok(off_t off, size_t len)
{
	return (len == 0U) ||
	       (((uint32_t)off < FLASH_SIZE) && (len <= FLASH_SIZE - (uint32_t)off));
}

static int flash_asr6502_read(const struct device *dev, off_t off, void *dst, size_t len)
{
	ARG_UNUSED(dev);
	if (!range_ok(off, len)) {
		return -EINVAL;
	}
	memcpy(dst, (const void *)(FLASH_BASE + (uint32_t)off), len);
	return 0;
}

static int flash_asr6502_write(const struct device *dev, off_t off, const void *src, size_t len)
{
	struct flash_asr6502_data *data = dev->data;
	const uint8_t *s = src;
	int ret = 0;

	if (!range_ok(off, len)) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	while (len > 0U) {
		uint32_t row_addr = (uint32_t)off & ~(FLASH_ROW - 1U);
		uint32_t pos = (uint32_t)off - row_addr;
		uint32_t n = MIN(len, FLASH_ROW - pos);
		uint8_t row[FLASH_ROW];

		/* Program-only (0x06) cannot clear bits, so merge over the
		 * current row contents: unchanged bytes re-program identically.
		 */
		memcpy(row, (const void *)(FLASH_BASE + row_addr), FLASH_ROW);
		memcpy(&row[pos], s, n);
		ret = flash_row(row_addr, row, OP_PROG_ROW);
		if (ret != 0) {
			break;
		}
		off += n;
		s += n;
		len -= n;
	}
	k_sem_give(&data->lock);
	return ret;
}

static int flash_asr6502_erase(const struct device *dev, off_t off, size_t len)
{
	struct flash_asr6502_data *data = dev->data;
	static const uint8_t zeros[FLASH_ROW];
	int ret = 0;

	if (((uint32_t)off % FLASH_ROW) != 0U || (len % FLASH_ROW) != 0U ||
	    !range_ok(off, len)) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	for (uint32_t a = (uint32_t)off; len > 0U; a += FLASH_ROW, len -= FLASH_ROW) {
		ret = flash_row(a, zeros, OP_WRITE_ROW);   /* erase -> 0x00 */
		if (ret != 0) {
			break;
		}
	}
	k_sem_give(&data->lock);
	return ret;
}

static const struct flash_parameters flash_asr6502_params = {
	.write_block_size = 4U,
	.erase_value = FLASH_ERASE_VALUE,
};

static const struct flash_parameters *flash_asr6502_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);
	return &flash_asr6502_params;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static const struct flash_pages_layout flash_asr6502_layout = {
	.pages_count = FLASH_SIZE / FLASH_ROW,
	.pages_size = FLASH_ROW,
};

static void flash_asr6502_page_layout(const struct device *dev,
				      const struct flash_pages_layout **layout, size_t *count)
{
	ARG_UNUSED(dev);
	*layout = &flash_asr6502_layout;
	*count = 1;
}
#endif

static DEVICE_API(flash, flash_asr6502_api) = {
	.read = flash_asr6502_read,
	.write = flash_asr6502_write,
	.erase = flash_asr6502_erase,
	.get_parameters = flash_asr6502_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_asr6502_page_layout,
#endif
};

static struct flash_asr6502_data flash_asr6502_data0;

static int flash_asr6502_init(const struct device *dev)
{
	struct flash_asr6502_data *data = dev->data;

	k_sem_init(&data->lock, 1, 1);
	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_asr6502_init, NULL, &flash_asr6502_data0, NULL,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_asr6502_api);
