#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Native device controls with a concurrent ordinary Linux CPU thread.

Build the static ARM64 guest with musl-gcc -static -O2 -pthread, linking
native-device-controls.c/.S and worker-gpu-native.S. The optional native macOS
GPU reference uses the same C source with DEVICE_NATIVE_GPU and the same
MoltenVK archive. Video surface metadata has no equivalent native Linux/macOS
API and is deliberately omitted from native reference timing.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
for name in ('baseline', 'candidate', 'fixture', 'rootfs', 'output'):
    p.add_argument('--' + name, type=Path, required=True)
p.add_argument('--native', type=Path)
p.add_argument('--repeats', type=int, default=3)
p.add_argument('--video-operations', type=int, default=2000)
p.add_argument('--gpu-operations', type=int, default=20)
a = p.parse_args()
assert a.repeats > 0 and min(a.video_operations, a.gpu_operations) > 0
a.output = a.output.resolve()
a.output.mkdir(parents=True, exist_ok=False)
env = dict(os.environ, TRAMES_MAC_BUNDLED='1', MVK_CONFIG_LOG_LEVEL='0')
report = dict(passed=False, scope=__doc__, records=[], sha256={
    n: hashlib.sha256(getattr(a, n).read_bytes()).hexdigest()
    for n in ('baseline', 'candidate', 'fixture', 'rootfs', 'native') if getattr(a, n)})


def save():
    (a.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')


def options(runner, cpus):
    return [str(runner.resolve()), '--storage', str(a.output / 'store'),
            '--no-pty', '--no-windows', '--network', 'off', '--video', '--vulkan',
            '--cpus', str(cpus), '--timeout', '60']


def run(mode, count, cpus, version, repeat):
    label = f'{mode}-{cpus}-{version}-{repeat}'
    if version == 'native':
        command = [str(a.native.resolve()), mode, str(count)]
    else:
        command = options(a.baseline if version == 'before' else a.candidate, cpus)
        if repeat == 0:
            command.append('--metrics')
        command += ['/device-bench', mode, str(count)]
    began = time.monotonic()
    r = subprocess.run(command, capture_output=True, env=env, timeout=70)
    wall = time.monotonic() - began
    (a.output / (label + '.stdout')).write_bytes(r.stdout)
    (a.output / (label + '.stderr')).write_bytes(r.stderr)
    assert r.returncode == 0, (label, r.returncode, r.stderr[-2000:])
    value = json.loads(r.stdout)
    assert value['mode'] == mode and value['operations'] == count and value['peer_calls'] > 0
    row = dict(mode=mode, cpus=cpus, version=version, repeat=repeat,
               wall_seconds=wall, result=value)
    if repeat == 0:
        prefix = 'trames mac execution paths: '
        lines = r.stderr.decode().splitlines()
        row['paths'] = next(json.loads(s[len(prefix):]) for s in lines if s.startswith(prefix))
        row['native_returns'] = next(s for s in lines if s.startswith('trames mac cohort-return native: '))
        if version == 'after' and cpus > 1:
            assert row['native_returns'].endswith('{}'), row
            assert row['paths']['service_requests'] == row['paths']['serviced_syscalls'] == 0
            if mode == 'gpu':
                assert row['paths']['worker_gpu_calls'] == (count + 1) * 5
    report['records'].append(row)
    save()
    print(label, value['seconds'], flush=True)


try:
    setup = subprocess.run(options(a.candidate, 4) + ['--rootfs', str(a.rootfs.resolve()),
        '--exec-file', '/device-bench=' + str(a.fixture.resolve()), '/bin/true'],
        env=env, capture_output=True, timeout=120)
    (a.output / 'setup.stderr').write_bytes(setup.stderr)
    assert setup.returncode == 0, setup.stderr
    for mode, count in (('video', a.video_operations), ('gpu', a.gpu_operations)):
        for cpus in (1, 4):
            for version in ('before', 'after'):
                run(mode, count, cpus, version, 0)
        for repeat in range(1, a.repeats + 1):
            for version in (('before', 'after') if repeat % 2 else ('after', 'before')):
                run(mode, count, 4, version, repeat)
            if mode == 'gpu' and a.native:
                run(mode, count, 4, 'native', repeat)
    report['summary'] = {}
    for mode in ('video', 'gpu'):
        report['summary'][mode] = {}
        for version in ('before', 'after', 'native'):
            rows = [r for r in report['records'] if r['mode'] == mode
                    and r['version'] == version and r['repeat'] > 0]
            if rows:
                report['summary'][mode][version] = dict(
                    loop_median_seconds=statistics.median(r['result']['seconds'] for r in rows),
                    wall_median_seconds=statistics.median(r['wall_seconds'] for r in rows))
    report['passed'] = True
    print(json.dumps(report['summary'], indent=2))
finally:
    save()
