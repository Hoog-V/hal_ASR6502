# Copyright (c) 2025 hal_ASR6502 contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_ASR6502_SDK)
  zephyr_library()

  zephyr_include_directories(include)

  zephyr_library_sources(
    src/bsp_common.c
    src/bsp_clocks.c
    src/bsp_delay.c
    src/bsp_io.c
    src/bsp_irq.c
    src/bsp_sar.c
    src/system_asr6502.c
  )

  zephyr_library_sources_ifdef(CONFIG_GPIO_ASR6502 drivers/gpio/gpio_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_ADC_ASR6502 drivers/adc/adc_asr6502.c)
  # adc_context.h lives in the Zephyr ADC driver tree, not a global include dir.
  zephyr_include_directories_ifdef(CONFIG_ADC_ASR6502 ${ZEPHYR_BASE}/drivers/adc)
  zephyr_library_sources_ifdef(CONFIG_COUNTER_ASR6502 drivers/counter/counter_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_HWINFO_ASR6502 drivers/hwinfo/hwinfo_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_WDT_ASR6502 drivers/watchdog/wdt_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_PINCTRL drivers/pinctrl/pinctrl_asr6502.c)
endif()
