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

/* Serial Communication Blocks (SCB): one block every 0x10000, instances 0..4.
 * Each SCB's clock input is PERI PCLK index n and can use divider n.
 */
#define ASR_SCB_BASE(n)           (0x40240000UL + ((n) * 0x10000UL))
#define ASR_SCB_COUNT             (5U)
#define ASR_SCB_INSTANCE(base)    (((base) - 0x40240000UL) / 0x10000UL)

#define ASR_SCB_CTRL              (0x0000UL) /* OVS[3:0], MODE[25:24], ENABLED bit31 */
#define ASR_SCB_SPI_CTRL          (0x0020UL) /* CONTINUOUS b0, CPHA b2, CPOL b3, MASTER b31 */
#define ASR_SCB_SPI_STATUS        (0x0024UL) /* BUS_BUSY bit0 */
#define ASR_SCB_UART_CTRL         (0x0040UL) /* MODE[25:24] (0 = standard UART) */
#define ASR_SCB_UART_TX_CTRL      (0x0044UL) /* STOP_BITS[2:0] */
#define ASR_SCB_UART_RX_CTRL      (0x0048UL) /* STOP_BITS[2:0] */
#define ASR_SCB_TX_CTRL           (0x0200UL) /* DATA_WIDTH[3:0] */
#define ASR_SCB_TX_FIFO_CTRL      (0x0204UL)
#define ASR_SCB_TX_FIFO_STATUS    (0x0208UL) /* USED[8:0] */
#define ASR_SCB_TX_FIFO_WR        (0x0240UL)
#define ASR_SCB_RX_CTRL           (0x0300UL) /* DATA_WIDTH[3:0] */
#define ASR_SCB_RX_FIFO_CTRL      (0x0304UL)
#define ASR_SCB_RX_FIFO_STATUS    (0x0308UL) /* USED[8:0] */
#define ASR_SCB_RX_FIFO_RD        (0x0340UL)

#define ASR_SCB_CTRL_MODE_UART    (2UL << 24)
#define ASR_SCB_CTRL_MODE_SPI     (1UL << 24)
#define ASR_SCB_CTRL_ENABLED_Msk  (0x80000000UL)
#define ASR_SCB_FIFO_USED_Msk     (0x1FFUL)
#define ASR_SCB_FIFO_DEPTH        (8U)       /* conservative lower bound */

#define ASR_SCB_SPI_CTRL_CONTINUOUS_Msk (0x1UL)
#define ASR_SCB_SPI_CTRL_CPHA_Msk       (0x4UL)
#define ASR_SCB_SPI_CTRL_CPOL_Msk       (0x8UL)
#define ASR_SCB_SPI_CTRL_MASTER_Msk     (0x80000000UL)
#define ASR_SCB_SPI_STATUS_BUS_BUSY_Msk (0x1UL)
#define ASR_SCB_CTRL_MSB_FIRST_Msk      (0x100UL) /* bit 8 in TX_CTRL/RX_CTRL */

/* PERI programmable clock dividers (from ASR_PERI_BASE). Divider divides the
 * input (HFCLK) by (value + 1); 16.0 integer type = 1.
 */
#define ASR_PERI_DIV_CMD          (0x0000UL)
#define ASR_PERI_PCLK_CTL(i)      (0x0100UL + ((i) * 4UL))
#define ASR_PERI_DIV_16_CTL(i)    (0x0300UL + ((i) * 4UL))
#define ASR_PERI_DIV_16_INT_Pos   (8U)       /* INT16_DIV[23:8] */
#define ASR_PERI_DIV_TYPE_16      (1UL)
#define ASR_PERI_DIV_CMD_ENABLE_Msk (0x80000000UL)
#define ASR_PERI_PCLK_SEL_TYPE_Pos  (6U)
#define ASR_PERI_DIV_CMD_SEL_TYPE_Pos (6U)
#define ASR_PERI_DIV_CMD_PA_SEL_DIV_Pos  (8U)
#define ASR_PERI_DIV_CMD_PA_SEL_TYPE_Pos (14U)
#define ASR_PERI_DIV_CMD_PA_NONE  ((0x3FUL << 8) | (3UL << 14)) /* no phase align */

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

/* SAR field positions/masks. */
#define ASR_SAR_CTRL_VREF_SEL_Pos          (4U)
#define ASR_SAR_CTRL_VREF_SEL_Msk          (0x70UL)
#define ASR_SAR_CTRL_NEG_SEL_Pos           (9U)
#define ASR_SAR_CTRL_NEG_SEL_Msk           (0xE00UL)
#define ASR_SAR_CTRL_ENABLED_Msk           (0x80000000UL)

#define ASR_SAR_SAMPLE_CTRL_SUB_RES_Pos    (0U)      /* 0=12-bit, else reduced */
#define ASR_SAR_SAMPLE_CTRL_LEFT_ALIGN_Msk (0x2UL)
#define ASR_SAR_SAMPLE_CTRL_SE_SIGNED_Msk  (0x4UL)
#define ASR_SAR_SAMPLE_CTRL_DIFF_SIGNED_Msk (0x8UL)
#define ASR_SAR_SAMPLE_CTRL_AVG_CNT_Pos    (4U)
#define ASR_SAR_SAMPLE_CTRL_AVG_CNT_Msk    (0x70UL)
#define ASR_SAR_SAMPLE_CTRL_AVG_SHIFT_Msk  (0x80UL)
#define ASR_SAR_SAMPLE_CTRL_CONTINUOUS_Msk (0x10000UL)

#define ASR_SAR_CHAN_CONFIG_PIN_ADDR_Pos   (0U)
#define ASR_SAR_CHAN_CONFIG_PIN_ADDR_Msk   (0x7UL)
#define ASR_SAR_CHAN_CONFIG_PORT_ADDR_Pos  (4U)
#define ASR_SAR_CHAN_CONFIG_PORT_ADDR_Msk  (0x70UL)
#define ASR_SAR_CHAN_CONFIG_DIFF_EN_Msk    (0x100UL)
#define ASR_SAR_CHAN_CONFIG_RESOLUTION_Msk (0x200UL) /* 1 = use reduced sub-resolution */
#define ASR_SAR_CHAN_CONFIG_AVG_EN_Msk     (0x400UL)
#define ASR_SAR_CHAN_CONFIG_ST_SEL_Pos     (12U)
#define ASR_SAR_CHAN_CONFIG_ST_SEL_Msk     (0x3000UL)

#define ASR_SAR_SAMPLE_TIME0_Msk           (0x3FFUL)
#define ASR_SAR_INTR_EOS_Msk               (0x1UL)
#define ASR_SAR_MUX_VSSA_VMINUS_Msk        (0x10000UL)

/***********************************************************************************************************************
 * SRSSLT clock registers (offsets from ASR_SRSSLT_BASE)
 **********************************************************************************************************************/

#define ASR_SRSSLT_PWR_CONTROL        (0x0000UL) /* Power-mode control/status */
#define ASR_SRSSLT_CLK_SELECT         (0x0028UL) /* HFCLK source/divider, SYSCLK divider */
#define ASR_SRSSLT_CLK_ILO_CONFIG     (0x002CUL) /* ILO enable (bit31) */
#define ASR_SRSSLT_CLK_IMO_CONFIG     (0x0030UL) /* IMO enable */
#define ASR_SRSSLT_WDT_DISABLE_KEY    (0x0038UL) /* Write 0xACED8865 to disable WDT reset */
#define ASR_SRSSLT_WDT_COUNTER        (0x003CUL) /* Free-running 16-bit ILO counter (RO) */
#define ASR_SRSSLT_WDT_MATCH          (0x0040UL) /* MATCH[15:0], IGNORE_BITS[19:16] */
#define ASR_SRSSLT_SRSS_INTR          (0x0044UL) /* WDT_MATCH bit0 (W1C) */
#define ASR_SRSSLT_SRSS_INTR_MASK     (0x004CUL) /* WDT_MATCH interrupt mask bit0 */
#define ASR_SRSSLT_RES_CAUSE          (0x0054UL) /* Reset cause */

#define ASR_WDT_DISABLE_KEY           (0xACED8865UL)
#define ASR_SRSS_INTR_WDT_MATCH_Msk   (0x1UL)
#define ASR_RES_CAUSE_WDT_Msk         (0x1UL)
#define ASR_RES_CAUSE_PROT_FAULT_Msk  (0x8UL)
#define ASR_RES_CAUSE_SOFT_Msk        (0x10UL)

/* SFLASH identification. */
#define ASR_SFLASH_SILICON_ID         (0x0144UL)
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
#define ASR_CLK_ILO_CONFIG_ENABLE_Msk (0x80000000UL)

/* ILO (internal low-speed oscillator) nominal frequency. */
#define ASR_CLK_ILO_FREQ_HZ           (40000UL)

/* SFLASH IMO trim tables (uint8_t[25], indexed by (imo_hz - 24 MHz) / 1 MHz). */
#define ASR_SFLASH_IMO_TCTRIM_LT      (0x01CCUL)
#define ASR_SFLASH_IMO_TRIM_LT        (0x01E5UL)

#ifdef __cplusplus
}
#endif

#endif /* ASR6502_H */
