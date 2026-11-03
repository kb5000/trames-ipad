#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Legacy AIO and FASYNC/epoll behavior against native Linux, on one and four CPUs."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
fixtures = ROOT / 'tests/fixtures/bin'
runner = os.getenv('TRAMES_RUNNER', str(ROOT / 'target/release/trames-run'))
reference = subprocess.run([str(fixtures / 'async-io-host')], capture_output=True, timeout=30)
assert reference.returncode == 0 and not reference.stderr, (reference.returncode, reference.stdout, reference.stderr)
with tempfile.TemporaryDirectory(prefix='trames-async-io-') as folder:
    for cpus in ('1', '4'):
        actual = subprocess.run([runner, '--storage', str(Path(folder) / cpus), '--cpus', cpus,
            '--rootfs', str(ROOT / 'target/alpine-download/alpine-minirootfs-3.22.6-aarch64.tar.gz'),
            '--max-instructions', '1000000000', str(fixtures / 'async-io')],
            capture_output=True, timeout=90)
        assert actual.returncode == 0 and actual.stdout == reference.stdout and not actual.stderr, (cpus, actual.returncode, actual.stdout, actual.stderr[-5000:])
        print(f'{cpus}-CPU ARM64 asynchronous I/O cases match native Linux', flush=True)
print(reference.stdout.decode(), end='')
