#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Guest-only BusyBox poweroff while an independent child keeps computing."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runner', type=Path, required=True)
parser.add_argument('--alpine', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--cpus', type=int, nargs='+', default=[1, 4, 12])
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
report = dict(passed=False, runner_sha256=hashlib.sha256(args.runner.read_bytes()).hexdigest(), cases=[])
try:
    for cpus in args.cpus:
        with tempfile.TemporaryDirectory(prefix='trames-guest-power-') as directory:
            root = Path(directory) / 'rootfs'
            root.mkdir()
            subprocess.run(['tar', '-xf', str(args.alpine.resolve()), '-C', str(root)], check=True)
            command = [str(args.runner.resolve()), '--rootfs', str(root), '--cpus', str(cpus),
                       '--network', 'off', '--no-pty', '--no-windows', '--metrics', '--timeout', '20',
                       '/bin/sh', '-c', 'while :; do :; done & echo POWER_BEGIN; /bin/busybox poweroff -f; echo POWER_RETURNED; exit 99']
            result = subprocess.run(command, stdin=subprocess.DEVNULL, capture_output=True,
                                    env=dict(os.environ, TRAMES_MAC_BUNDLED='1'), timeout=30)
            (args.output / f'{cpus}.stdout').write_bytes(result.stdout)
            (args.output / f'{cpus}.stderr').write_bytes(result.stderr)
            row = dict(cpus=cpus, code=result.returncode)
            report['cases'].append(row)
            assert result.returncode == 0, (row, result.stderr[-3000:])
            assert b'POWER_BEGIN' in result.stdout and b'POWER_RETURNED' not in result.stdout, result.stdout
            prefix = 'trames mac execution paths: '
            metrics = json.loads(next(line[len(prefix):] for line in result.stderr.decode().splitlines() if line.startswith(prefix)))
            row['metrics'] = metrics
            prefix = 'trames mac cohort-return syscalls: '
            returned = ast.literal_eval(next(line[len(prefix):] for line in result.stderr.decode().splitlines() if line.startswith(prefix)))
            row['cohort_return_syscalls'] = returned
            assert not any(number == 142 and count for number, count in returned), row
            assert metrics['service_requests'] == metrics['serviced_syscalls'] == 0, row
    report['passed'] = True
finally:
    (args.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(dict(passed=report['passed'], cases=len(report['cases']))))
