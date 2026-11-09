#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare traced/untraced original syscall execution and accepted arguments."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--backend', choices=('hvf', 'interpreter', 'native'), required=True)
p.add_argument('--runner', type=Path)
p.add_argument('--fixture', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--cpus', type=int, nargs='+', default=(1, 4, 12))
p.add_argument('--modes', nargs='+', choices=('queries', 'mixed'), default=('queries', 'mixed'))
p.add_argument('--trace', choices=('both', 'on', 'off'), default='both')
p.add_argument('--rounds', type=int, default=20)
p.add_argument('--baseline', action='store_true')
a = p.parse_args()
assert 0 < a.rounds < 1 << 20
if a.backend != 'native' and a.runner is None:
    p.error('--runner is required for guest backends')
a.output.mkdir(parents=True, exist_ok=True)
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
report = dict(backend=a.backend, fixture_sha256=sha(a.fixture), passed=False, cases=[])
if a.runner:
    report['runner_sha256'] = sha(a.runner)
pattern = re.compile(r'^trames pid (\d+) syscall (\d+)\(([^)]+)\) = (.+)$', re.M)
try:
    for cpus in ([0] if a.backend == 'native' else a.cpus):
        for mode in a.modes:
            for traced in ([False] if a.backend == 'native' else
                           [False, True] if a.trace == 'both' else [a.trace == 'on']):
                label = f'{mode}-{cpus}-' + ('trace' if traced else 'plain')
                command = [str(a.fixture.resolve()), mode, str(a.rounds)]
                if a.backend != 'native':
                    empty = a.output / 'rootfs'
                    (empty / 'tmp').mkdir(parents=True, exist_ok=True)
                    command = [str(a.runner.resolve()), '--cpus', str(cpus), '--network', 'off',
                               '--metrics', '--exec-file', f'/fixture={a.fixture.resolve()}']
                    command += (['--rootfs', str(empty.resolve()), '--no-pty', '--no-windows', '--timeout', '60']
                                if a.backend == 'hvf' else ['--memory', '--guest', '--accelerator', 'pure',
                                                            '--max-instructions', '3000000000'])
                    if traced:
                        command += ['--trace' if a.backend == 'hvf' else '--trace-syscalls']
                    command += ['/fixture', mode, str(a.rounds)]
                row = dict(cpus=cpus, mode=mode, traced=traced, command=command)
                report['cases'].append(row)
                with (a.output / (label + '.stdout')).open('wb') as output, \
                        (a.output / (label + '.stderr')).open('wb') as errors:
                    result = subprocess.run(command, stdout=output, stderr=errors, timeout=75,
                                            stdin=subprocess.DEVNULL,
                                            env=dict(os.environ, TRAMES_MAC_BUNDLED='1'))
                row['code'] = result.returncode
                assert result.returncode == 0, (row, (a.output / (label + '.stderr')).read_text()[-4000:])
                measured = json.loads((a.output / (label + '.stdout')).read_text().strip().splitlines()[-1])
                row['measurement'] = measured
                assert measured['failure'] == 0 and measured['rounds'] == a.rounds
                assert measured['signals'] == (2 if mode == 'mixed' else 0)
                errors = (a.output / (label + '.stderr')).read_text()
                if a.backend == 'native':
                    print(label, 'PASS', flush=True)
                    continue
                events = pattern.findall(errors)
                assert bool(events) == traced, (label, len(events))
                if traced:
                    seen = {}
                    handlers = []
                    metadata = []
                    for tid, number, arguments, value in events:
                        args = [int(x, 16) for x in arguments.split(', ')]
                        if number == '172' and (0x5a000000 <= args[0] < 0x5a400000 or
                                                args[0] in (0x7e000001, 0x7e000002) or
                                                0x6a000000 <= args[0] < 0x6a000004 or
                                                0x6b000000 <= args[0] < 0x6b000004):
                            assert args[1:] == [0x1122, 0x3344, 0x5566, 0x7788, 0x99aa], (label, arguments)
                            assert int(value) == measured['pid'], (label, arguments, value)
                            if args[0] < 0x5a400000:
                                assert args[0] not in seen, (label, 'duplicate accepted call', args[0])
                                seen[args[0]] = int(tid)
                            elif args[0] >= 0x7e000000:
                                handlers.append(args[0])
                            else:
                                metadata.append(args[0])
                    expected = {0x5a000000 + (thread << 20) + i for thread in range(4) for i in range(a.rounds)}
                    assert set(seen) == expected, (label, len(seen), len(expected), list(expected - set(seen))[:5])
                    assert sorted(handlers) == ([0x7e000001, 0x7e000002] if mode == 'mixed' else [])
                    row['verified_marked_calls'] = len(seen)
                    row['verified_handler_calls'] = len(handlers)
                    expected_metadata = sorted(base + thread for base in (0x6a000000, 0x6b000000) for thread in range(4)) if mode == 'mixed' else []
                    assert sorted(metadata) == expected_metadata, (label, 'metadata completion consumed a later entry', metadata)
                    row['verified_metadata_calls'] = len(metadata)
                    row['restart_records'] = sum(value == 'restarted' for _, _, _, value in events)
                    if mode == 'mixed' and not a.baseline:
                        assert row['restart_records'] >= 1, (label, 'missing original restarted read')
                        reads = [value for _, number, _, value in events if number == '63']
                        assert reads.count('1') == 2 and reads.count('-4') == 1, (label, 'read completion was attributed to sigreturn', reads)
                prefix = 'trames mac execution paths: ' if a.backend == 'hvf' else 'trames metrics: '
                metrics = json.loads(next(line[len(prefix):] for line in errors.splitlines() if line.startswith(prefix)))
                row['metrics'] = metrics
                assert metrics['service_requests'] == metrics['serviced_syscalls'] == 0
                if a.backend == 'hvf':
                    prefix = 'trames mac cohort-return syscalls: '
                    returned = ast.literal_eval(next(line[len(prefix):] for line in errors.splitlines() if line.startswith(prefix)))
                    row['cohort_return_syscalls'] = returned
                    if cpus > 1 and not a.baseline:
                        assert not returned, row
                        assert metrics['cohort_rounds'] == 1, row
                print(label, 'PASS', flush=True)
    report['passed'] = True
finally:
    (a.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
