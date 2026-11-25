# CI image for building the HAL against the LHT52 board. Bakes in Canonical's
# Zephyr SDK NG 1.0.1 (ARM) and a Zephyr 4.5 workspace with the three modules an
# LHT52 image needs, so CI runs are a pull plus a compile. Built by
# ci/build-image.sh / .github/workflows/image.yml; the context's sdk/ and
# workspace/ dirs are assembled there, not committed.
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
      git cmake ninja-build gperf ccache dfu-util device-tree-compiler \
      python3 python3-pip python3-venv wget xz-utils file make \
      libncurses6 libisl23 libmpfr6 libmpc3 libzstd1 \
    && rm -rf /var/lib/apt/lists/*

# Canonical Zephyr SDK NG 1.0.1 (ARM toolchain + SDK metadata).
COPY sdk/ /opt/zephyr-sdk/
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr

# Baked west workspace (Zephyr + modules). hal_ASR6502, the manifest self
# project, is copied into /workspace/hal_ASR6502 by the build job.
COPY workspace/ /workspace/
ENV ZEPHYR_BASE=/workspace/zephyr

RUN pip3 install --break-system-packages --no-cache-dir west \
    && pip3 install --break-system-packages --no-cache-dir \
       -r /workspace/zephyr/scripts/requirements-base.txt

WORKDIR /workspace
