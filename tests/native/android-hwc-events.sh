#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
set -euo pipefail
cd "$(dirname "$0")/../.."
temporary="$(mktemp -d "${TMPDIR:-/tmp}/trames-hwc-events.XXXXXX")"
trap 'rm -rf "$temporary"' EXIT
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -pthread tests/native/android-hwc-events.c -o "$temporary/events"
"$temporary/events"
