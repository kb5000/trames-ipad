#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checked, counterbalanced namespace lifecycles on two direct Mac runners."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--before', type=Path, required=True)
p.add_argument('--after', type=Path, required=True)
p.add_argument('--fixture', type=Path, required=True)
p.add_argument('--native-darwin', type=Path)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--samples', type=int, default=3)
p.add_argument('--iterations', type=int, default=1000)
p.add_argument('--workers', type=int, nargs='+', default=[1, 2, 4])
p.add_argument('--storage', choices=['memory', 'private'], nargs='+', default=['memory', 'private'])
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=False)
(a.output / 'empty-rootfs').mkdir()
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
report = dict(passed=False, fixture_sha256=sha(a.fixture),
              runners={key: sha(path) for key, path in [('before', a.before), ('after', a.after)]},
              parameters=dict(samples=a.samples, iterations=a.iterations, workers=a.workers, storage=a.storage), rows=[], native=[])
pattern = re.compile(rb'^namespace-worker (lifecycle) threads=(\d+) iterations=(\d+) longest_ns=(\d+)$', re.M)


def run(label, runner, storage, mode, workers, sample, diagnostics=False):
    name = f'{storage}-{mode}-{workers}-{sample}-{label}' + ('-metrics' if diagnostics else '')
    case = a.output / name
    case.mkdir()
    command = [str(runner.resolve()), '--rootfs', str((a.output / 'empty-rootfs').resolve()),
               '--cpus', str(workers), '--network', 'off', '--timeout', '30', '--no-pty', '--no-windows']
    if storage == 'private':
        command += ['--storage', str((case / 'store').resolve())]
    if diagnostics:
        command += ['--metrics']
    command += ['--exec-file', f'/file-bench={a.fixture.resolve()}', '/file-bench', mode, str(workers), str(a.iterations)]
    began = time.monotonic()
    result = subprocess.run(command, capture_output=True, timeout=40)
    wall = time.monotonic() - began
    (case / 'stdout').write_bytes(result.stdout)
    (case / 'stderr').write_bytes(result.stderr)
    match = pattern.search(result.stdout)
    assert result.returncode == 0 and match, (name, result.returncode, result.stdout, result.stderr[-2500:])
    assert match[1].decode() == mode and int(match[2]) == workers and int(match[3]) == a.iterations
    row = dict(storage=storage, mode=mode, workers=workers, runner=label, sample=sample,
               diagnostics=diagnostics, longest_ns=int(match[4]), wall_seconds=wall)
    if diagnostics:
        prefix = 'trames mac execution paths: '
        row['paths'] = json.loads(next(line[len(prefix):] for line in result.stderr.decode().splitlines() if line.startswith(prefix)))
    report['rows'].append(row)
    print(json.dumps(row), flush=True)


try:
    for storage in a.storage:
        for mode in ['lifecycle']:
            for workers in a.workers:
                for sample in range(a.samples):
                    order = [('before', a.before), ('after', a.after)]
                    if sample % 2:
                        order.reverse()
                    for label, runner in order:
                        run(label, runner, storage, mode, workers, sample)
    for storage in a.storage:
        for mode in ['lifecycle']:
            for workers in a.workers:
                for label, runner in [('before', a.before), ('after', a.after)]:
                    run(label, runner, storage, mode, workers, 'diagnostic', True)
    if a.native_darwin:
        report['native_darwin_sha256'] = sha(a.native_darwin)
        for mode in ['lifecycle']:
            for workers in a.workers:
                for sample in range(a.samples):
                    case = a.output / f'native-darwin-{mode}-{workers}-{sample}'
                    case.mkdir()
                    result = subprocess.run([str(a.native_darwin.resolve()), mode, str(workers), str(a.iterations)],
                                            cwd=case, capture_output=True, timeout=30)
                    (case / 'stdout').write_bytes(result.stdout)
                    (case / 'stderr').write_bytes(result.stderr)
                    match = pattern.search(result.stdout)
                    assert result.returncode == 0 and match, (case, result.stderr)
                    report['native'].append(dict(mode=mode, workers=workers, sample=sample, longest_ns=int(match[4])))
    medians = []
    for storage in a.storage:
        for mode in ['lifecycle']:
            for workers in a.workers:
                row = dict(storage=storage, mode=mode, workers=workers)
                for label in ['before', 'after']:
                    samples = [r for r in report['rows'] if r['storage'] == storage and r['mode'] == mode
                               and r['workers'] == workers and r['runner'] == label and not r['diagnostics']]
                    row[label + '_ns'] = statistics.median(r['longest_ns'] for r in samples)
                    row[label + '_wall_seconds'] = statistics.median(r['wall_seconds'] for r in samples)
                row['speedup'] = row['before_ns'] / row['after_ns']
                native = [r['longest_ns'] for r in report['native'] if r['mode'] == mode and r['workers'] == workers]
                if native:
                    row['native_darwin_ns'] = statistics.median(native)
                medians.append(row)
    report.update(passed=True, medians=medians,
                  method=f'{a.samples} counterbalanced unprofiled samples; separate diagnostic runs. Parent directory creation, thread creation, barrier setup and process startup/teardown are outside the longest-worker timer. Child directory/file creation, bytes, metadata, links, rename, stat, unlink and close remain inside the checked lifecycle timer.',
                  native_limit='Darwin reference uses its filesystem APIs; no native RAM-filesystem score is substituted. Linux reference, when supplied separately, uses the identical static ARM64 ELF on the local ARM Linux CI instance.')
finally:
    (a.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
