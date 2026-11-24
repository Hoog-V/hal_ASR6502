/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Clock control for the ASR6502.
 */

#include "bsp_clocks.h"
#include "bsp_delay.h"   /* asr_delay_cycles */
#include "asr6502.h"     /* SystemCoreClock, SRSSLT/SFLASH register map, CMSIS */

#define ASR_SRSSLT_REG(off)       (*(volatile uint32_t *) (ASR_SRSSLT_BASE + (off)))
#define ASR_SFLASH_TRIM(tbl, idx) (((volatile uint8_t *) (ASR_SFLASH_BASE + (tbl)))[(idx)])

#define ASR_CLK_IMO_BASE_HZ       (24000000UL)   /* IMO value for FREQ field 0 */

uint32_t asr_clock_sysclk_calc(const asr_clock_config_t *cfg)
{
    if (cfg == (const asr_clock_config_t *) 0) {
        return 0U;
    }

    /* IMO frequencies are multiples of 4 MHz and the dividers are powers of two,
     * so each shift is exact. */
    uint32_t hfclk = (uint32_t) cfg->imo_freq >> (uint32_t) cfg->hfclk_div;

    return hfclk >> (uint32_t) cfg->sysclk_div;
}

/* Live IMO frequency in Hz, or 0 if the IMO is disabled. */
static uint32_t asr_clock_imo_freq_hz(void)
{
    if ((ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_CONFIG) & ASR_CLK_IMO_CONFIG_ENABLE_Msk) == 0U) {
        return 0U;
    }

    uint32_t field = ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_SELECT) & ASR_CLK_IMO_SELECT_FREQ_Msk;

    return ((field << 2) * 1000000UL) + ASR_CLK_IMO_BASE_HZ;
}

void asr_clock_sysclk_update(void)
{
    uint32_t sel = ASR_SRSSLT_REG(ASR_SRSSLT_CLK_SELECT);
    uint32_t src;

    /* Only the IMO HFCLK source is resolvable here; an external clock's rate is
     * not known to the HAL. */
    if (((sel & ASR_CLK_SELECT_HFCLK_SEL_Msk) >> ASR_CLK_SELECT_HFCLK_SEL_Pos) != 0U) {
        return;
    }

    src = asr_clock_imo_freq_hz();

    uint32_t hfclk_div  = (sel & ASR_CLK_SELECT_HFCLK_DIV_Msk) >> ASR_CLK_SELECT_HFCLK_DIV_Pos;
    uint32_t sysclk_div = (sel & ASR_CLK_SELECT_SYSCLK_DIV_Msk) >> ASR_CLK_SELECT_SYSCLK_DIV_Pos;
    uint32_t sysclk     = (src >> hfclk_div) >> sysclk_div;

    if (sysclk != 0U) {
        SystemCoreClock = sysclk;
    }
}

void asr_clock_imo_set_freq(asr_clock_imo_freq_t freq)
{
    /* IMO must be running to be retrimmed. */
    if ((ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_CONFIG) & ASR_CLK_IMO_CONFIG_ENABLE_Msk) == 0U) {
        return;
    }

    /* SFLASH trim-table index for this frequency (0, 4, 8, ... 24). */
    uint32_t idx   = ((uint32_t) freq - ASR_CLK_IMO_BASE_HZ) / 1000000UL;
    uint32_t field = idx >> 2;   /* FREQ bitfield value, 0..6 */

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    /* Drop to 24 MHz, apply this frequency's coarse / fine / TC trim. */
    ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_SELECT) = 0U;
    ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_TRIM1)  = ASR_SFLASH_TRIM(ASR_SFLASH_IMO_TRIM_LT, idx);
    ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_TRIM2)  = 0U;
    ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_TRIM3)  = ASR_SFLASH_TRIM(ASR_SFLASH_IMO_TCTRIM_LT, idx);

    asr_delay_cycles(50U);

    /* Step up through the adjacent frequency to avoid an overshoot. */
    if (field != 0U) {
        ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_SELECT) = (field - 1U) & ASR_CLK_IMO_SELECT_FREQ_Msk;
        asr_delay_cycles(50U);
        ASR_SRSSLT_REG(ASR_SRSSLT_CLK_IMO_SELECT) = field & ASR_CLK_IMO_SELECT_FREQ_Msk;
    }

    __set_PRIMASK(primask);

    asr_clock_sysclk_update();
}

uint32_t asr_clock_sysclk_get(void)
{
    return SystemCoreClock;
}

uint32_t asr_clock_hfclk_get(void)
{
    uint32_t sel = ASR_SRSSLT_REG(ASR_SRSSLT_CLK_SELECT);
    uint32_t hfclk_div;

    if (((sel & ASR_CLK_SELECT_HFCLK_SEL_Msk) >> ASR_CLK_SELECT_HFCLK_SEL_Pos) != 0U) {
        return 0U;   /* external source: rate unknown to the HAL */
    }

    hfclk_div = (sel & ASR_CLK_SELECT_HFCLK_DIV_Msk) >> ASR_CLK_SELECT_HFCLK_DIV_Pos;

    return asr_clock_imo_freq_hz() >> hfclk_div;
}

void asr_clock_ilo_enable(bool enable)
{
    if (enable) {
        ASR_SRSSLT_REG(ASR_SRSSLT_CLK_ILO_CONFIG) |= ASR_CLK_ILO_CONFIG_ENABLE_Msk;
    } else {
        ASR_SRSSLT_REG(ASR_SRSSLT_CLK_ILO_CONFIG) &= ~ASR_CLK_ILO_CONFIG_ENABLE_Msk;
    }
}

bool asr_clock_ilo_is_enabled(void)
{
    return (ASR_SRSSLT_REG(ASR_SRSSLT_CLK_ILO_CONFIG) & ASR_CLK_ILO_CONFIG_ENABLE_Msk) != 0U;
}
