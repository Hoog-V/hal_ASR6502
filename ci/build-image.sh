#!/usr/bin/env bash
#
# Assemble and build (and optionally push) the CI image: Canonical's Zephyr
# SDK NG 1.0.1 (ARM) plus a shallow Zephyr 4.5 workspace, baked into an image
# the build workflow just pulls and compiles in.
#
# Run from the workshop project dir (the one holding .workshop/ and
# hal_ASR6502/) with a launched workshop named "ci" (workshop launch ci).
# Works locally (podman) or in CI (docker).
#
#   IMAGE=ghcr.io/<owner>/hal_asr6502-ci:latest  target image ref
#   CONTAINER_TOOL=docker|podman                 defaults to docker
#   PUSH=1                                        also push after build
#
set -euo pipefail

IMAGE="${IMAGE:-ghcr.io/hoog-v/hal_asr6502-ci:latest}"
TOOL="${CONTAINER_TOOL:-docker}"
SDK=/var/lib/workshop/sdk/zephyr-sdk-ng/zephyr-sdk
CTX="$(mktemp -d)"
trap 'rm -rf "$CTX"' EXIT

echo ">> Extracting ARM Zephyr SDK from the workshop"
mkdir -p "$CTX/sdk/gnu"
# Container root can read the idmapped SDK mounts; stream tarballs out via the
# project mount, then unpack on the host.
workshop exec -I -- sudo tar -czf /project/_sdk-meta.tgz \
  -C "$SDK" cmake sdk_version sdk_toolchains setup.sh
workshop exec -I -- sudo tar -czf /project/_sdk-tc.tgz \
  -C "$SDK/gnu" arm-zephyr-eabi
tar xzf _sdk-meta.tgz -C "$CTX/sdk"
tar xzf _sdk-tc.tgz -C "$CTX/sdk/gnu"
rm -f _sdk-meta.tgz _sdk-tc.tgz

echo ">> Fetching the Zephyr workspace (shallow)"
workshop run -I -- setup

echo ">> Assembling the workspace layer (source only, no .git)"
mkdir -p "$CTX/workspace/.west"
printf '[manifest]\npath = hal_ASR6502\nfile = west.yml\n[zephyr]\nbase = zephyr\n' \
  > "$CTX/workspace/.west/config"
rsync -a --exclude='.git' zephyr "$CTX/workspace/"
for m in modules/hal/cmsis_6 modules/lib/picolibc modules/debug/segger; do
  mkdir -p "$CTX/workspace/$(dirname "$m")"
  rsync -a --exclude='.git' "$m" "$CTX/workspace/$(dirname "$m")/"
done

echo ">> Building $IMAGE with $TOOL"
cp hal_ASR6502/ci/Dockerfile "$CTX/Dockerfile"
"$TOOL" build -t "$IMAGE" "$CTX"

if [ "${PUSH:-0}" = "1" ]; then
  echo ">> Pushing $IMAGE"
  "$TOOL" push "$IMAGE"
fi
echo ">> Done"
