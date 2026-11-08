#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Identical AArch64 job-control ELF on native Linux, HVF and the iPad interpreter."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--backend', choices=['native', 'hvf', 'interpreter'], required=True)
parser.add_argument('--runner', type=Path)
parser.add_argument('--fixture', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--cpus', type=int, nargs='+', default=[1, 4, 12])
parser.add_argument('--modes', nargs='+', choices=['compat', 'queries'], default=['compat', 'queries'])
parser.add_argument('--rounds', type=int, default=1000)
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
empty = args.output / 'empty-rootfs'
empty.mkdir(exist_ok=True)
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
report = dict(backend=args.backend, fixture_sha256=sha(args.fixture), passed=False, cases=[])
if args.backend != 'native':
    if args.runner is None:
        parser.error('--runner is required for a runtime backend')
    report['runner_sha256'] = sha(args.runner)
try:
    for cpus in args.cpus:
        for mode in args.modes:
            if args.backend == 'native':
                command = [str(args.fixture.resolve())]
            else:
                command = [str(args.runner.resolve()), '--cpus', str(cpus), '--network', 'off',
                           '--metrics', '--exec-file', f'/job-control-session={args.fixture.resolve()}']
                command += (['--rootfs', str(empty.resolve()), '--timeout', '60', '--no-pty', '--no-windows']
                            if args.backend == 'hvf' else ['--memory', '--guest'])
                command += ['/job-control-session']
            command += [mode, str(args.rounds)]
            began = time.monotonic()
            result = subprocess.run(command, env=dict(os.environ, TRAMES_MAC_BUNDLED='1'),
                                    capture_output=True, timeout=70)
            name = f'{mode}-{cpus}'
            (args.output / (name + '.stdout')).write_bytes(result.stdout)
            (args.output / (name + '.stderr')).write_bytes(result.stderr)
            row = dict(cpus=cpus, mode=mode, returncode=result.returncode, wall_seconds=time.monotonic()-began)
            report['cases'].append(row)
            assert result.returncode == 0, (name, result.stdout, result.stderr[:3000])
            if mode == 'compat':
                assert result.stdout.endswith(b'detach and final hangup: ok\n'), result.stdout
            else:
                value = json.loads(result.stdout.decode().splitlines()[-1])
                assert value['mode'] == 'queries' and value['threads'] == 4 and value['rounds'] == args.rounds
                row['measurement'] = value
            prefix = 'trames mac execution paths: ' if args.backend == 'hvf' else 'trames metrics: '
            if args.backend != 'native':
                metrics = json.loads(next(line[len(prefix):] for line in result.stderr.decode().splitlines() if line.startswith(prefix)))
                assert metrics['service_requests'] == 0, metrics
                if cpus > 1:
                    assert metrics['worker_direct_syscalls'] > 0, metrics
                    if mode == 'queries':
                        assert metrics['worker_direct_syscalls'] >= 4*args.rounds*4, metrics
                row['metrics'] = metrics
                if args.backend == 'hvf':
                    returned = next(line for line in result.stderr.decode().splitlines() if line.startswith('trames mac cohort-return syscalls: '))
                    row['cohort_return_syscalls'] = ast.literal_eval(returned.split(': ', 1)[1])
                    assert not any(number in (29, 56, 63, 64, 65, 66, 154, 155, 156, 157)
                                   for number, _ in row['cohort_return_syscalls']), row
            print(name, 'PASS', flush=True)
    report['passed'] = True
finally:
    (args.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
