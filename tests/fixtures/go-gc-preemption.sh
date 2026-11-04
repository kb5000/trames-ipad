#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Stock Debian Go-built Docker CLI; loopback/server/site data are unnecessary.
set -eu
export PATH=/usr/sbin:/usr/bin:/sbin:/bin
export GOGC=1
for n in $(seq 1 30); do
 printf 'GO_GC_START %s\n' "$n"
 timeout -s QUIT -k 3 10 docker --help >/run/go-gc-help.out 2>/run/go-gc-help.err || { cat /run/go-gc-help.err; exit 1; }
 printf 'GO_GC_PASS %s\n' "$n"
done
