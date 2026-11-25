# Copyright (c) 2025 hal_ASR6502 contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HAS_ASR6502_SDK)
  zephyr_library()

  zephyr_include_directories(include)

  # Always needed: SoC startup/clock bring-up and the glitch-free IMO retrim delay.
  zephyr_library_sources(
    src/bsp_clocks.c
    src/bsp_delay.c
    src/system_asr6502.c
  )

  # HAL support compiled only for the drivers that use it.
  zephyr_library_sources_ifdef(CONFIG_GPIO_ASR6502 src/bsp_io.c)   # drive mode / HSIOM
  zephyr_library_sources_ifdef(CONFIG_ADC_ASR6502  src/bsp_sar.c)  # SAR ADC

  # bsp_irq.c and bsp_common.c expose helpers nothing currently calls; leave them
  # out of the build so they cost no flash (re-add here if a driver starts using them).

  zephyr_library_sources_ifdef(CONFIG_GPIO_ASR6502 drivers/gpio/gpio_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_ADC_ASR6502 drivers/adc/adc_asr6502.c)
  # adc_context.h lives in the Zephyr ADC driver tree, not a global include dir.
  zephyr_include_directories_ifdef(CONFIG_ADC_ASR6502 ${ZEPHYR_BASE}/drivers/adc)
  zephyr_library_sources_ifdef(CONFIG_COUNTER_ASR6502 drivers/counter/counter_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_HWINFO_ASR6502 drivers/hwinfo/hwinfo_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_WDT_ASR6502 drivers/watchdog/wdt_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_UART_ASR6502 drivers/serial/uart_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_PINCTRL drivers/pinctrl/pinctrl_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_FLASH_ASR6502 drivers/flash/flash_asr6502.c)
  zephyr_library_sources_ifdef(CONFIG_SPI_ASR6502 drivers/spi/spi_asr6502.c)
  # spi_context.h lives in the Zephyr SPI driver tree, not a global include dir.
  zephyr_include_directories_ifdef(CONFIG_SPI_ASR6502 ${ZEPHYR_BASE}/drivers/spi)
endif()
