/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Version types for the ASR6502 HAL.
 */

#ifndef ASR_VERSION_H
#define ASR_VERSION_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include <stdint.h>

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/* Pack version numbers (major.minor.patch.build). */
#define ASR_VERSION_MAJOR    (0U)
#define ASR_VERSION_MINOR    (1U)
#define ASR_VERSION_PATCH    (0U)
#define ASR_VERSION_BUILD    (0U)

/* Aliases consumed by the version structures below. */
#define ASR_VERSION_CODE_VERSION_MAJOR    (ASR_VERSION_MAJOR)
#define ASR_VERSION_CODE_VERSION_MINOR    (ASR_VERSION_MINOR)

/***********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/* Implementation/interface version reported by a driver, readable as one word or by field. */
typedef union st_asr_version
{
    uint32_t version_id;               /* Whole value */
    struct
    {
        uint8_t code_version_minor;    /* Implementation minor number */
        uint8_t code_version_major;    /* Implementation major number */
        uint8_t api_version_minor;     /* Interface minor number */
        uint8_t api_version_major;     /* Interface major number */
    };
} asr_version_t;

/* HAL release version, readable as one word or by field. */
typedef union st_asr_pack_version
{
    uint32_t version_id;               /* Whole value */
    struct
    {
        uint8_t build;                 /* Build field */
        uint8_t patch;                 /* Patch field */
        uint8_t minor;                 /* Minor field */
        uint8_t major;                 /* Major field */
    };
} asr_pack_version_t;

#endif                                 /* ASR_VERSION_H */
