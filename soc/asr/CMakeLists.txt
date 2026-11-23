# Copyright (c) 2025 hal_ASR6502 contributors
# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

zephyr_library()
zephyr_library_sources(soc.c)
zephyr_include_directories(.)
