#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Run on AArch64 Linux, or supply a target CC and matching PKG_CONFIG/sysroot.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
out=${1:-"$root/target/audio-guest"}
mkdir -p "$out"
${CC:-cc} -O2 -g -fPIC -DPIC -shared -Wall -Wextra -Werror \
  $(${PKG_CONFIG:-pkg-config} --cflags alsa) \
  "$root/runtime/crates/trames-audio/guest/alsa.c" \
  "$root/runtime/crates/trames-audio/guest/calls.S" \
  $(${PKG_CONFIG:-pkg-config} --libs alsa) \
  -Wl,-z,noexecstack -o "$out/libasound_module_pcm_trames.so"
cp "$root/runtime/crates/trames-audio/guest/asound.conf" "$out/asound.conf"
${CC:-cc} -O2 -g -Wall -Wextra -Werror \
  "$root/tests/fixtures/audio-native.c" \
  $(${PKG_CONFIG:-pkg-config} --cflags --libs alsa) -lm -lpthread \
  -o "$out/audio-native"
cp "$root/LICENSE" "$out/GPL-3.0.txt"
