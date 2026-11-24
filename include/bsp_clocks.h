/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Clock control for the ASR6502. The system clock is
 *
 *     SYSCLK = (IMO >> hfclk_div) >> sysclk_div
 *
 * where the IMO (internal main oscillator) runs at 24..48 MHz in 4 MHz steps and
 * each divider is a power of two (/1, /2, /4, /8). SYSCLK feeds SystemCoreClock.
 */

#ifndef BSP_CLOCKS_H
#define BSP_CLOCKS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* IMO (HFCLK source) frequency, in Hz. */
typedef enum
{
    ASR_CLOCK_IMO_24MHZ = 24000000UL,
    ASR_CLOCK_IMO_28MHZ = 28000000UL,
    ASR_CLOCK_IMO_32MHZ = 32000000UL,
    ASR_CLOCK_IMO_36MHZ = 36000000UL,
    ASR_CLOCK_IMO_40MHZ = 40000000UL,
    ASR_CLOCK_IMO_44MHZ = 44000000UL,
    ASR_CLOCK_IMO_48MHZ = 48000000UL,
} asr_clock_imo_freq_t;

/* Clock divider: output = input >> value (0 = /1, 1 = /2, 2 = /4, 3 = /8). */
typedef enum
{
    ASR_CLOCK_DIV_1 = 0,
    ASR_CLOCK_DIV_2 = 1,
    ASR_CLOCK_DIV_4 = 2,
    ASR_CLOCK_DIV_8 = 3,
} asr_clock_div_t;

/* Description of an IMO-sourced clock tree. */
typedef struct
{
    asr_clock_imo_freq_t imo_freq;   /* IMO frequency */
    asr_clock_div_t      hfclk_div;  /* HFCLK = IMO >> hfclk_div */
    asr_clock_div_t      sysclk_div; /* SYSCLK = HFCLK >> sysclk_div */
} asr_clock_config_t;

/* Pure calculation: SYSCLK (Hz) for a configuration. No hardware access. */
uint32_t asr_clock_sysclk_calc(const asr_clock_config_t *cfg);

/* Read the live SRSSLT clock registers and refresh SystemCoreClock. Handles the
 * IMO HFCLK source; returns leaving SystemCoreClock unchanged for an external
 * source whose frequency is not known to the HAL. */
void asr_clock_sysclk_update(void);

/* Program the IMO to the given frequency (loads the SFLASH trim and performs the
 * glitch-free step sequence), then refresh SystemCoreClock. No effect if the IMO
 * is disabled.
 *
 * Warning: under Zephyr the SysTick rate is fixed at boot from
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC (the 48 MHz boot default). Retrimming the
 * IMO after the timer is running breaks cycle-based timing; only call this
 * function before scheduler/timer init or disable it for clock changes that
 * affect the core. */
void asr_clock_imo_set_freq(asr_clock_imo_freq_t freq);

/* Current SYSCLK (SystemCoreClock) in Hz. */
uint32_t asr_clock_sysclk_get(void);

/* HFCLK frequency in Hz (IMO after the HFCLK divider, ahead of the SYSCLK divider);
 * 0 for an external HFCLK source whose rate is not known to the HAL. */
uint32_t asr_clock_hfclk_get(void);

/* Enable or disable the ILO (internal low-speed oscillator, ~40 kHz). The ILO
 * clocks the watchdog and is available as a wake source in deep sleep. */
void asr_clock_ilo_enable(bool enable);

/* True if the ILO is running. */
bool asr_clock_ilo_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CLOCKS_H */
