/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SoC declaration for the ASR6502 (ASR Micro LoRa SoC: Arm Cortex-M0+ / PSoC 4100S
 * core + integrated Semtech SX126x radio). Core configuration, memory map and
 * peripheral base addresses only; no peripheral register definitions.
 */

#ifndef ASR6502_H
#define ASR6502_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
 * Interrupt number definition
 **********************************************************************************************************************/

typedef enum IRQn
{
    /* Cortex-M0+ processor exceptions */
    NonMaskableInt_IRQn   = -14,       /* Non-maskable interrupt */
    HardFault_IRQn        = -13,       /* Hard fault */
    SVCall_IRQn           = -5,        /* Supervisor call */
    PendSV_IRQn           = -2,        /* Pendable service call */
    SysTick_IRQn          = -1,        /* System tick timer */

    /*
     * Device interrupts occupy NVIC lines 0..31. On this core the sources are
     * steered to these lines by the programmable interrupt mux, so the mapping is
     * configured at runtime. Generic line identifiers are provided here; rename
     * each to its source from the ASR6502 reference manual as it is wired up.
     */
    DeviceLine0_IRQn      = 0,
    DeviceLine1_IRQn      = 1,
    DeviceLine2_IRQn      = 2,
    DeviceLine3_IRQn      = 3,
    DeviceLine4_IRQn      = 4,
    DeviceLine5_IRQn      = 5,
    DeviceLine6_IRQn      = 6,
    DeviceLine7_IRQn      = 7,
    DeviceLine8_IRQn      = 8,
    DeviceLine9_IRQn      = 9,
    DeviceLine10_IRQn     = 10,
    DeviceLine11_IRQn     = 11,
    DeviceLine12_IRQn     = 12,
    DeviceLine13_IRQn     = 13,
    DeviceLine14_IRQn     = 14,
    DeviceLine15_IRQn     = 15,
    DeviceLine16_IRQn     = 16,
    DeviceLine17_IRQn     = 17,
    DeviceLine18_IRQn     = 18,
    DeviceLine19_IRQn     = 19,
    DeviceLine20_IRQn     = 20,
    DeviceLine21_IRQn     = 21,
    DeviceLine22_IRQn     = 22,
    DeviceLine23_IRQn     = 23,
    DeviceLine24_IRQn     = 24,
    DeviceLine25_IRQn     = 25,
    DeviceLine26_IRQn     = 26,
    DeviceLine27_IRQn     = 27,
    DeviceLine28_IRQn     = 28,
    DeviceLine29_IRQn     = 29,
    DeviceLine30_IRQn     = 30,
    DeviceLine31_IRQn     = 31,
} IRQn_Type;

#define ASR_IRQ_LINE_COUNT    (32U)

/***********************************************************************************************************************
 * Processor and core peripheral configuration (CMSIS)
 **********************************************************************************************************************/

#define __CM0PLUS_REV             0x0001U   /* Core revision r0p1 */
#define __MPU_PRESENT             0         /* No MPU */
#define __VTOR_PRESENT            1         /* Vector table relocation supported (table lives in SRAM) */
#define __NVIC_PRIO_BITS          2         /* 2 priority bits */
#define __Vendor_SysTickConfig    0         /* Use the CMSIS SysTick configuration */
#define __FPU_PRESENT             0         /* No FPU */

#include <core_cm0plus.h>
#include "system_asr6502.h"

/***********************************************************************************************************************
 * Clock
 **********************************************************************************************************************/

/* Default core clock used until SystemCoreClockUpdate() reads the live configuration.
 * Set to the reset value of the internal main oscillator; adjust for the board. */
#ifndef ASR_DEFAULT_CORE_CLOCK_HZ
 #define ASR_DEFAULT_CORE_CLOCK_HZ    (48000000UL)
#endif

/***********************************************************************************************************************
 * Memory map
 **********************************************************************************************************************/

#define ASR_FLASH_BASE            (0x00000000UL)   /* Program flash */
#define ASR_FLASH_SIZE            (0x00020000UL)   /* 128 KB */

#define ASR_SRAM_BASE             (0x20000000UL)   /* SRAM */
#define ASR_SRAM_SIZE             (0x00004000UL)   /* 16 KB (initial SP at top) */

#define ASR_PERIPHERAL_BASE       (0x40000000UL)   /* Peripheral region */

/***********************************************************************************************************************
 * Peripheral base addresses
 **********************************************************************************************************************/

#define ASR_PERI_BASE             (0x40010000UL)   /* Peripheral interconnect / clock dividers */
#define ASR_HSIOM_BASE            (0x40020000UL)   /* High-speed I/O matrix (pin routing) */
#define ASR_GPIO_BASE             (0x40040000UL)   /* GPIO ports */
#define ASR_SAR_BASE              (0x403A0000UL)   /* SAR ADC */
#define ASR_SRSSLT_BASE           (0x40030000UL)   /* SRSS-lite: clock, power, WDT */
#define ASR_CPUSS_BASE            (0x40100000UL)   /* CPU subsystem */
#define ASR_SFLASH_BASE           (0x0FFFF000UL)   /* Supervisory flash (trim/config) */

/* GPIO: one port block every 0x100 bytes, ports 0..7. */
#define ASR_GPIO_PRT_SIZE         (0x00000100UL)
#define ASR_GPIO_PRT_BASE(port)   (ASR_GPIO_BASE + ((port) * ASR_GPIO_PRT_SIZE))
#define ASR_GPIO_PORT_COUNT       (8U)

/* HSIOM: one port block every 0x100 bytes; PORT_SEL0 holds 8 pins x 4 bits. */
#define ASR_HSIOM_PRT_SIZE        (0x00000100UL)
#define ASR_HSIOM_PRT_BASE(port)  (ASR_HSIOM_BASE + ((port) * ASR_HSIOM_PRT_SIZE))
#define ASR_HSIOM_PORT_SEL0       (0x00UL)   /* 4 bits per pin */

/* GPIO per-port register offsets. */
#define ASR_GPIO_DR               (0x00UL)   /* Output data */
#define ASR_GPIO_PS               (0x04UL)   /* Input state */
#define ASR_GPIO_PC               (0x08UL)   /* Drive mode / port control */
#define ASR_GPIO_INTR_CFG         (0x0CUL)   /* Edge select */
#define ASR_GPIO_INTR             (0x10UL)   /* Interrupt status */

/* SAR ADC register offsets (from ASR_SAR_BASE). CHAN_CONFIG/CHAN_RESULT are [16]. */
#define ASR_SAR_CTRL              (0x0000UL) /* Analog control: ENABLED bit31, VREF_SEL[6:4] */
#define ASR_SAR_SAMPLE_CTRL       (0x0004UL) /* Sample control: CONTINUOUS bit16 */
#define ASR_SAR_SAMPLE_TIME01     (0x0010UL) /* Sample times ST0 [9:0], ST1 [25:16] */
#define ASR_SAR_CHAN_EN           (0x0020UL) /* Per-channel enable bits */
#define ASR_SAR_START_CTRL        (0x0024UL) /* FW_TRIGGER bit0 (one-shot start) */
#define ASR_SAR_CHAN_CONFIG       (0x0080UL) /* [16]: PIN_ADDR[3:0], PORT_ADDR[7:4], AVG_EN bit10 */
#define ASR_SAR_CHAN_RESULT       (0x0180UL) /* [16]: RESULT[15:0] */
#define ASR_SAR_INTR              (0x0210UL) /* EOS_INTR bit0 (end of scan) */
#define ASR_SAR_INTR_MASK         (0x0218UL) /* EOS interrupt mask bit0 */
#define ASR_SAR_MUX_SWITCH0       (0x0300UL) /* FW switches: Pn->Vplus bit n, VSSA->Vminus bit16 */

/***********************************************************************************************************************
 * SRSSLT clock registers (offsets from ASR_SRSSLT_BASE)
 **********************************************************************************************************************/

#define ASR_SRSSLT_CLK_SELECT         (0x0028UL) /* HFCLK source/divider, SYSCLK divider */
#define ASR_SRSSLT_CLK_IMO_CONFIG     (0x0030UL) /* IMO enable */
#define ASR_SRSSLT_CLK_IMO_SELECT     (0x0F08UL) /* IMO frequency field */
#define ASR_SRSSLT_CLK_IMO_TRIM1      (0x0F0CUL) /* IMO coarse trim */
#define ASR_SRSSLT_CLK_IMO_TRIM2      (0x0F10UL) /* IMO fine trim */
#define ASR_SRSSLT_CLK_IMO_TRIM3      (0x0F18UL) /* IMO temperature-coefficient trim */

#define ASR_CLK_SELECT_HFCLK_SEL_Pos  (0U)
#define ASR_CLK_SELECT_HFCLK_SEL_Msk  (0x3UL)
#define ASR_CLK_SELECT_HFCLK_DIV_Pos  (2U)
#define ASR_CLK_SELECT_HFCLK_DIV_Msk  (0xCUL)
#define ASR_CLK_SELECT_SYSCLK_DIV_Pos (6U)
#define ASR_CLK_SELECT_SYSCLK_DIV_Msk (0xC0UL)
#define ASR_CLK_IMO_SELECT_FREQ_Pos   (0U)
#define ASR_CLK_IMO_SELECT_FREQ_Msk   (0x7UL)
#define ASR_CLK_IMO_CONFIG_ENABLE_Msk (0x80000000UL)

/* SFLASH IMO trim tables (uint8_t[25], indexed by (imo_hz - 24 MHz) / 1 MHz). */
#define ASR_SFLASH_IMO_TCTRIM_LT      (0x01CCUL)
#define ASR_SFLASH_IMO_TRIM_LT        (0x01E5UL)

#ifdef __cplusplus
}
#endif

#endif /* ASR6502_H */
