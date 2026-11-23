/*
 * Copyright (c) 2025 hal_ASR6502 contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Cortex-M0+ startup for the ASR6502 SoC (GCC). Provides the vector table, the
 * reset handler (.data copy, .bss clear, SystemInit, static ctors, main) and weak
 * default handlers for every exception and device interrupt line. Override any
 * handler by defining a function of the same name.
 *
 * Expects the standard GCC/newlib linker symbols: __etext, __data_start__,
 * __data_end__, __bss_start__, __bss_end__, __StackTop, __libc_init_array, main.
 */

#include <stdint.h>

/* Linker-provided symbols. */
extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __StackTop;

extern void __libc_init_array(void);
extern int  main(void);
extern void SystemInit(void);

/* Each handler is a weak alias of Default_Handler unless overridden elsewhere. */
#define ALIAS_DEFAULT    __attribute__((weak, alias("Default_Handler")))

void Reset_Handler(void);
void Default_Handler(void);

/* Cortex-M0+ core exceptions. */
void NMI_Handler(void)        ALIAS_DEFAULT;
void HardFault_Handler(void)  ALIAS_DEFAULT;
void SVC_Handler(void)        ALIAS_DEFAULT;
void PendSV_Handler(void)     ALIAS_DEFAULT;
void SysTick_Handler(void)    ALIAS_DEFAULT;

/* Device interrupt lines 0..31 (rename to their sources as they are wired up). */
void DeviceLine0_Handler(void)  ALIAS_DEFAULT;
void DeviceLine1_Handler(void)  ALIAS_DEFAULT;
void DeviceLine2_Handler(void)  ALIAS_DEFAULT;
void DeviceLine3_Handler(void)  ALIAS_DEFAULT;
void DeviceLine4_Handler(void)  ALIAS_DEFAULT;
void DeviceLine5_Handler(void)  ALIAS_DEFAULT;
void DeviceLine6_Handler(void)  ALIAS_DEFAULT;
void DeviceLine7_Handler(void)  ALIAS_DEFAULT;
void DeviceLine8_Handler(void)  ALIAS_DEFAULT;
void DeviceLine9_Handler(void)  ALIAS_DEFAULT;
void DeviceLine10_Handler(void) ALIAS_DEFAULT;
void DeviceLine11_Handler(void) ALIAS_DEFAULT;
void DeviceLine12_Handler(void) ALIAS_DEFAULT;
void DeviceLine13_Handler(void) ALIAS_DEFAULT;
void DeviceLine14_Handler(void) ALIAS_DEFAULT;
void DeviceLine15_Handler(void) ALIAS_DEFAULT;
void DeviceLine16_Handler(void) ALIAS_DEFAULT;
void DeviceLine17_Handler(void) ALIAS_DEFAULT;
void DeviceLine18_Handler(void) ALIAS_DEFAULT;
void DeviceLine19_Handler(void) ALIAS_DEFAULT;
void DeviceLine20_Handler(void) ALIAS_DEFAULT;
void DeviceLine21_Handler(void) ALIAS_DEFAULT;
void DeviceLine22_Handler(void) ALIAS_DEFAULT;
void DeviceLine23_Handler(void) ALIAS_DEFAULT;
void DeviceLine24_Handler(void) ALIAS_DEFAULT;
void DeviceLine25_Handler(void) ALIAS_DEFAULT;
void DeviceLine26_Handler(void) ALIAS_DEFAULT;
void DeviceLine27_Handler(void) ALIAS_DEFAULT;
void DeviceLine28_Handler(void) ALIAS_DEFAULT;
void DeviceLine29_Handler(void) ALIAS_DEFAULT;
void DeviceLine30_Handler(void) ALIAS_DEFAULT;
void DeviceLine31_Handler(void) ALIAS_DEFAULT;

typedef void (*vector_entry_t)(void);

const vector_entry_t __vector_table[] __attribute__((section(".isr_vector"), used)) =
{
    (vector_entry_t) &__StackTop,      /* Initial stack pointer */
    Reset_Handler,                     /* Reset */
    NMI_Handler,                       /* NMI */
    HardFault_Handler,                 /* Hard fault */
    0, 0, 0, 0, 0, 0, 0,               /* Reserved */
    SVC_Handler,                       /* SVCall */
    0, 0,                              /* Reserved */
    PendSV_Handler,                    /* PendSV */
    SysTick_Handler,                   /* SysTick */

    DeviceLine0_Handler,  DeviceLine1_Handler,  DeviceLine2_Handler,  DeviceLine3_Handler,
    DeviceLine4_Handler,  DeviceLine5_Handler,  DeviceLine6_Handler,  DeviceLine7_Handler,
    DeviceLine8_Handler,  DeviceLine9_Handler,  DeviceLine10_Handler, DeviceLine11_Handler,
    DeviceLine12_Handler, DeviceLine13_Handler, DeviceLine14_Handler, DeviceLine15_Handler,
    DeviceLine16_Handler, DeviceLine17_Handler, DeviceLine18_Handler, DeviceLine19_Handler,
    DeviceLine20_Handler, DeviceLine21_Handler, DeviceLine22_Handler, DeviceLine23_Handler,
    DeviceLine24_Handler, DeviceLine25_Handler, DeviceLine26_Handler, DeviceLine27_Handler,
    DeviceLine28_Handler, DeviceLine29_Handler, DeviceLine30_Handler, DeviceLine31_Handler,
};

void Reset_Handler(void)
{
    /* Copy initialised data from flash to SRAM. */
    uint32_t *src = &__etext;
    uint32_t *dst = &__data_start__;

    while (dst < &__data_end__) {
        *dst++ = *src++;
    }

    /* Zero the .bss section. */
    for (dst = &__bss_start__; dst < &__bss_end__; dst++) {
        *dst = 0U;
    }

    SystemInit();
    __libc_init_array();
    (void) main();

    /* main() should not return; trap if it does. */
    while (1) {
    }
}

void Default_Handler(void)
{
    while (1) {
    }
}
