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
    src/system_asr6502.c
  )
endif()
