/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Common BSP primitives for the ASR6502 (Cortex-M0+): version query, interrupt
 * critical sections, assertion/return helpers and the board init hook.
 */

#ifndef BSP_COMMON_H
#define BSP_COMMON_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "asr_common.h"           /* asr_err_t, asr_pack_version_t, ASR_* macros */
#include "asr6502.h"              /* CMSIS core intrinsics */

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/* Marker for a disabled interrupt slot. */
#define ASR_IRQ_DISABLED                       (0xFFU)

/* Number of Cortex-M0+ processor exceptions ahead of the device interrupt lines. */
#define ASR_CORTEX_PROCESSOR_EXCEPTIONS        (16U)

/*
 * Interrupt critical section. The Cortex-M0+ has only PRIMASK (no BASEPRI), so a
 * section masks all maskable interrupts and restores the previous mask on exit.
 * Usage:  ASR_CRITICAL_SECTION_DEFINE; ... ASR_CRITICAL_SECTION_ENTER; ... ASR_CRITICAL_SECTION_EXIT;
 */
#define ASR_CRITICAL_SECTION_DEFINE            uint32_t asr_saved_primask = 0U
#define ASR_CRITICAL_SECTION_ENTER                         \
    do {                                                   \
        asr_saved_primask = __get_PRIMASK();               \
        __disable_irq();                                   \
    } while (0)
#define ASR_CRITICAL_SECTION_EXIT              (__set_PRIMASK(asr_saved_primask))

/* Return err unless condition holds. */
#define ASR_ERROR_RETURN(condition, err)                   \
    do {                                                   \
        if (!(condition)) {                                \
            return (err);                                  \
        }                                                  \
    } while (0)

/* Runtime assertion (overridable by the application). */
#ifndef ASR_ASSERT
 #define ASR_ASSERT(a)    assert(a)
#endif

/**********************************************************************************************************************
 * Function prototypes
 **********************************************************************************************************************/

/* Return the HAL pack version. Returns ASR_ERR_INVALID_POINTER on a NULL argument. */
asr_err_t asr_version_get(asr_pack_version_t *const p_version);

/* Board/SoC bring-up hook, called once at startup. Weak; override to customise. */
void bsp_init(void *p_args);

#ifdef __cplusplus
}
#endif

#endif /* BSP_COMMON_H */
