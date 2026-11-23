/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Common BSP primitives for the ASR6502.
 */

#include "bsp_common.h"

/* HAL pack version, built from the macros in asr_version.h. */
static const asr_pack_version_t g_asr_version = {
    .major = ASR_VERSION_MAJOR,
    .minor = ASR_VERSION_MINOR,
    .patch = ASR_VERSION_PATCH,
    .build = ASR_VERSION_BUILD,
};

asr_err_t asr_version_get(asr_pack_version_t *const p_version)
{
    ASR_ERROR_RETURN(p_version != NULL, ASR_ERR_INVALID_POINTER);

    p_version->version_id = g_asr_version.version_id;

    return ASR_SUCCESS;
}

__attribute__((weak)) void bsp_init(void *p_args)
{
    ASR_PARAMETER_NOT_USED(p_args);
}

/*
 * Assertion back-ends. A failed assert() traps with interrupts masked so the
 * state can be inspected with a debugger. Override to report instead.
 */
void __aeabi_assert(const char *expr, const char *file, int line)
{
    ASR_PARAMETER_NOT_USED(expr);
    ASR_PARAMETER_NOT_USED(file);
    ASR_PARAMETER_NOT_USED(line);

    __disable_irq();
    while (1) {
    }
}

__attribute__((weak)) void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    ASR_PARAMETER_NOT_USED(file);
    ASR_PARAMETER_NOT_USED(line);
    ASR_PARAMETER_NOT_USED(func);
    ASR_PARAMETER_NOT_USED(expr);

    __disable_irq();
    while (1) {
    }
}
