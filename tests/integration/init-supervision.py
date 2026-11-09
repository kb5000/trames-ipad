#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real init notification/authentication and graceful stop during CPU execution."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import select
import shutil
import signal
import subprocess
import tempfile
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--backend', choices=('hvf', 'native'), required=True)
p.add_argument('--runner', type=Path)
p.add_argument('--alpine', type=Path)
p.add_argument('--fixture', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--cpus', type=int, nargs='+', default=(1, 4, 12))
p.add_argument('--baseline', action='store_true')
a = p.parse_args()
if a.backend == 'hvf' and (not a.runner or not a.alpine):
    p.error('--runner and --alpine are required for HVF')
a.output.mkdir(parents=True, exist_ok=True)
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
report = dict(backend=a.backend, fixture_sha256=sha(a.fixture), passed=False, cases=[])
if a.runner:
    report['runner_sha256'] = sha(a.runner)
try:
    for cpus in (a.cpus if a.backend == 'hvf' else [0]):
        row = dict(cpus=cpus)
        report['cases'].append(row)
        output = bytearray()
        started = time.monotonic()
        with tempfile.TemporaryDirectory(prefix='trames-init-supervision-') as temp:
            root = Path(temp) / 'rootfs'
            root.mkdir()
            if a.backend == 'hvf':
                subprocess.run(['tar', '-xf', str(a.alpine.resolve()), '-C', str(root)], check=True)
            image = root / 'usr/lib/systemd/systemd'
            image.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(a.fixture, image)
            image.chmod(0o755)
            command = [str(a.fixture.resolve()), 'native']
            if a.backend == 'hvf':
                command = [str(a.runner.resolve()), '--rootfs', str(root),
                           '--init', 'systemd', '--cpus', str(cpus), '--network', 'off',
                           '--no-pty', '--no-windows', '--metrics', '--timeout', '20']
            row['command'] = command
            errors_path = a.output / f'{cpus}.stderr'
            with errors_path.open('wb') as errors:
                child = subprocess.Popen(command, stdin=subprocess.DEVNULL,
                                         stdout=subprocess.PIPE, stderr=errors,
                                         env=dict(os.environ, TRAMES_MAC_BUNDLED='1'))
                os.set_blocking(child.stdout.fileno(), False)
                sent_stop = sent_native = False
                try:
                    while child.poll() is None:
                        assert time.monotonic() - started < 30, (row, output[-2000:])
                        if select.select([child.stdout], [], [], .02)[0]:
                            output.extend(os.read(child.stdout.fileno(), 65536))
                        if b'SUPERVISOR_START' in output and not sent_stop:
                            if a.backend == 'hvf':
                                child.send_signal(signal.SIGTERM)
                            sent_stop = True
                            row['host_stop_seconds'] = time.monotonic() - started
                        if a.backend == 'native' and b'SUPERVISOR_ARMED' in output and not sent_native:
                            child.send_signal(signal.SIGUSR1)
                            sent_native = True
                    output.extend(child.stdout.read() or b'')
                    row['code'] = child.wait(timeout=1)
                    row['elapsed_seconds'] = time.monotonic() - started
                    assert row['code'] == (143 if a.backend == 'hvf' else 0), (row, output, errors_path.read_text()[-3000:])
                finally:
                    if child.poll() is None:
                        child.kill()
                        child.wait()
                    child.stdout.close()
                    (a.output / f'{cpus}.stdout').write_bytes(output)
            assert b'SUPERVISOR_UNARMED_OK' in output and b'SUPERVISOR_ARMED' in output
            measured = json.loads(next(line for line in output.decode().splitlines() if line.startswith('{')))
            row['measurement'] = measured
            assert measured['passed'] and measured['progress'] > 0
            assert measured['signal_count'] == 1
            assert measured['signal'] == (37 if a.backend == 'hvf' else signal.SIGUSR1)
            if a.backend == 'hvf':
                assert measured['pid'] == 1
                errors = errors_path.read_text()
                assert errors.count('trames: systemd ready') == 1, errors
                prefix = 'trames mac execution paths: '
                metrics = json.loads(next(line[len(prefix):] for line in errors.splitlines() if line.startswith(prefix)))
                row['metrics'] = metrics
                assert metrics['service_requests'] == metrics['serviced_syscalls'] == 0
                prefix = 'trames mac cohort-return syscalls: '
                row['cohort_return_syscalls'] = ast.literal_eval(next(line[len(prefix):] for line in errors.splitlines() if line.startswith(prefix)))
                if cpus > 1 and not a.baseline:
                    assert metrics['cohort_rounds'] == 1, row
                    assert not row['cohort_return_syscalls'], row
            print(cpus, 'PASS', flush=True)
    report['passed'] = True
finally:
    (a.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
