/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SAR ADC driver for the ASR6502.
 */

#include "bsp_sar.h"
#include "asr6502.h"

#define SAR_REG(off)             (*(volatile uint32_t *) (ASR_SAR_BASE + (off)))
#define SAR_CHAN_CONFIG_REG(ch)  SAR_REG(ASR_SAR_CHAN_CONFIG + ((uint32_t) (ch) * 4UL))
#define SAR_CHAN_RESULT_REG(ch)  SAR_REG(ASR_SAR_CHAN_RESULT + ((uint32_t) (ch) * 4UL))

/* Register fields. */
#define SAR_CTRL_ENABLED_Msk        (0x80000000UL)
#define SAR_CTRL_VREF_SEL_Pos       (4U)
#define SAR_CTRL_VREF_SEL_Msk       (0x7UL)
#define SAR_SAMPLE_TIME0_Msk        (0x3FFUL)
#define SAR_CHAN_CONFIG_PIN_Pos     (0U)   /* SARMUX pin (PORT_ADDR left 0 = SARMUX) */
#define SAR_CHAN_CONFIG_PIN_Msk     (0x7UL)
#define SAR_START_CTRL_FW_TRIGGER   (0x1UL)
#define SAR_INTR_EOS                (0x1UL)
#define SAR_MUX_VSSA_VMINUS         (0x10000UL)   /* MUX_SWITCH0 bit 16 */

#define ASR_SAR_MAX_CHAN            (16U)   /* result channels */

/* Channel -> SARMUX pin, recorded at asr_sar_channel_config() time. */
static uint8_t sar_chan_pin[ASR_SAR_MAX_CHAN];

void asr_sar_init(uint8_t vref_sel, uint16_t sample_time)
{
    /* Shared sample aperture ST0. */
    SAR_REG(ASR_SAR_SAMPLE_TIME01) = (uint32_t) sample_time & SAR_SAMPLE_TIME0_Msk;

    /* Single-ended, software one-shot (CONTINUOUS = 0, unsigned result). */
    SAR_REG(ASR_SAR_SAMPLE_CTRL) = 0U;

    /*
     * Enable the SAR and select the reference. The remaining CTRL analog-bias
     * fields (VREF buffer power, negative-input select) are left at their reset
     * values; set them per the datasheet if absolute accuracy is required.
     */
    SAR_REG(ASR_SAR_CTRL) =
        SAR_CTRL_ENABLED_Msk |
        (((uint32_t) vref_sel & SAR_CTRL_VREF_SEL_Msk) << SAR_CTRL_VREF_SEL_Pos);
}

void asr_sar_channel_config(uint8_t chan, uint8_t sarmux_pin)
{
    if (chan >= ASR_SAR_MAX_CHAN) {
        return;
    }

    /* Positive input = SARMUX pin; port 0 = SARMUX; single-ended (no DIFFERENTIAL_EN). */
    SAR_CHAN_CONFIG_REG(chan) =
        ((uint32_t) sarmux_pin & SAR_CHAN_CONFIG_PIN_Msk) << SAR_CHAN_CONFIG_PIN_Pos;

    SAR_REG(ASR_SAR_CHAN_EN) |= (1UL << chan);

    /* Remember the input; the SARMUX switches are connected at read time
     * (MUX_SWITCH0 is shared routing — accumulating connections here would
     * couple external sources together during conversions). */
    sar_chan_pin[chan] = (uint8_t) (sarmux_pin & 0x7U);
}

int16_t asr_sar_read(uint8_t chan)
{
    if (chan >= ASR_SAR_MAX_CHAN) {
        return -1;
    }

    /* Connect only this channel's input (shared routing, set per conversion) and
     * the VSSA negative reference, then trigger the conversion.
     */
    SAR_REG(ASR_SAR_MUX_SWITCH0) = (1UL << sar_chan_pin[chan]) | SAR_MUX_VSSA_VMINUS;

    /* Clear any stale end-of-scan, trigger a conversion, wait for completion. */
    SAR_REG(ASR_SAR_INTR) = SAR_INTR_EOS;
    SAR_REG(ASR_SAR_START_CTRL) = SAR_START_CTRL_FW_TRIGGER;

    while ((SAR_REG(ASR_SAR_INTR) & SAR_INTR_EOS) == 0U) {
        /* busy-wait for end of scan */
    }

    SAR_REG(ASR_SAR_INTR) = SAR_INTR_EOS;

    return (int16_t) (SAR_CHAN_RESULT_REG(chan) & 0xFFFFU);
}
