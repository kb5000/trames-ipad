#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""A real pthread workload must exceed the former 256-task cap and join/reuse."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
fixture = Path(os.getenv('TRAMES_TASK_CAPACITY_FIXTURE', root / 'tests/fixtures/bin/task-capacity'))
native = Path(os.getenv('TRAMES_TASK_CAPACITY_NATIVE', root / 'tests/fixtures/bin/task-capacity-host'))
runner = os.getenv('TRAMES_RUNNER', str(root / 'target/release/trames-run'))
rootfs = Path(os.getenv('TRAMES_ALPINE_CACHE', root / 'target/alpine-download')) / 'alpine-minirootfs-3.22.6-aarch64.tar.gz'
if native.is_file():
    reference = subprocess.run([str(native), '512'], capture_output=True, timeout=120)
    assert reference.returncode == 0 and b'task-capacity-ok:' in reference.stdout, reference
with tempfile.TemporaryDirectory(prefix='trames-task-capacity-') as directory:
    result = subprocess.run([runner, '--storage', directory, '--rootfs', str(rootfs),
                             '--exec-file', '/tmp/task-capacity=' + str(fixture),
                             '--cpus', '4', '--max-instructions', '100000000000',
                             '--guest', '/tmp/task-capacity', '512'],
                            capture_output=True, timeout=180)
assert result.returncode == 0 and result.stdout.count(b'task-capacity-round:') == 4, result
assert b'task-capacity-ok:' in result.stdout, result
print(result.stdout.decode(), end='')
