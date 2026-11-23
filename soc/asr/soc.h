/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ASR_SOC_H_
#define ZEPHYR_SOC_ASR_SOC_H_

/* The device header must come first: it defines IRQn_Type and the CMSIS
 * feature macros (__MPU_PRESENT, __VTOR_PRESENT, ...) that the CMSIS core
 * headers (pulled in via <cmsis_core.h>) expect to be already visible. */
#include "asr6502.h"
#include <cmsis_core.h>

#endif /* ZEPHYR_SOC_ASR_SOC_H_ */
