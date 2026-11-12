#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Focused Sync2 oracle; the guest ICD and original loader are built separately.
set -euo pipefail
cd "$(dirname "$0")/.."
output="${1:-tests/fixtures/bin}"
mkdir -p "$output"
headers="$PWD/target/vulkan-upstream/headers/Vulkan-Headers-cfebfc96b2b0bce93da7d12f2c14cc01793ae25c/include"
"${GLSLANG_VALIDATOR:-glslangValidator}" -V --target-env vulkan1.2 tests/fixtures/vulkan-sync2.comp -o "$output/vulkan-sync2.spv"
"${SPIRV_VAL:-spirv-val}" --target-env vulkan1.2 "$output/vulkan-sync2.spv"
"${MUSL_CC:-$PWD/target/busybox-build/musl/bin/musl-gcc}" -O2 -Wall -Wextra -Werror -Wno-misleading-indentation \
    -I "$headers" -L "$PWD/target/vulkan-loader-sdk" -L "$PWD/target/alpine-build/lib" \
    tests/fixtures/vulkan-sync2.c -lvulkan -o "$output/vulkan-sync2"
