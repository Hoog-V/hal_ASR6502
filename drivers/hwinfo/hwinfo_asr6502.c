/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * hwinfo backend for the ASR6502: reset cause (SRSSLT RES_CAUSE) and a device
 * identifier (SFLASH silicon ID).
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <string.h>

#include "asr6502.h"

#define SRSSLT_RES_CAUSE  (*(volatile uint32_t *) (ASR_SRSSLT_BASE + ASR_SRSSLT_RES_CAUSE))
#define SFLASH_SILICON_ID (*(volatile uint32_t *) (ASR_SFLASH_BASE + ASR_SFLASH_SILICON_ID))

#define ASR_RESET_SUPPORTED  (RESET_WATCHDOG | RESET_SOFTWARE | RESET_SECURITY)

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	/*
	 * The SFLASH silicon ID identifies the part; it is not a per-die serial
	 * number (those registers are not exposed here). Reported big-endian.
	 */
	uint16_t id = (uint16_t) (SFLASH_SILICON_ID & 0xFFFFU);
	uint8_t dev_id[2] = { (uint8_t) (id >> 8), (uint8_t) id };

	if (length > sizeof(dev_id)) {
		length = sizeof(dev_id);
	}

	memcpy(buffer, dev_id, length);

	return (ssize_t) length;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t rc = SRSSLT_RES_CAUSE;
	uint32_t flags = 0U;

	if ((rc & ASR_RES_CAUSE_WDT_Msk) != 0U) {
		flags |= RESET_WATCHDOG;
	}
	if ((rc & ASR_RES_CAUSE_SOFT_Msk) != 0U) {
		flags |= RESET_SOFTWARE;
	}
	if ((rc & ASR_RES_CAUSE_PROT_FAULT_Msk) != 0U) {
		flags |= RESET_SECURITY;
	}

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	/* RES_CAUSE bits are write-1-to-clear. */
	SRSSLT_RES_CAUSE = ASR_RES_CAUSE_WDT_Msk | ASR_RES_CAUSE_SOFT_Msk |
			   ASR_RES_CAUSE_PROT_FAULT_Msk;

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = ASR_RESET_SUPPORTED;

	return 0;
}
