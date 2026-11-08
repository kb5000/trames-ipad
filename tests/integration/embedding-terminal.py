#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise original embedding PTY queries and input under blocked host output."""
import argparse
import ast
import errno
import hashlib
import json
import os
from pathlib import Path
import pty
import select
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--backend', choices=('hvf', 'interpreter', 'native'), required=True)
p.add_argument('--runner', type=Path)
p.add_argument('--fixture', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--cpus', nargs='+', type=int, default=(1, 4, 12))
p.add_argument('--modes', nargs='+', choices=('queries', 'duplex'), default=('queries', 'duplex'))
p.add_argument('--rounds', type=int, default=1000)
p.add_argument('--baseline', action='store_true')
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
if a.backend != 'native' and a.runner is None:
    p.error('--runner is required for guest backends')
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
report = dict(backend=a.backend, fixture_sha256=sha(a.fixture), passed=False, cases=[])
if a.runner:
    report['runner_sha256'] = sha(a.runner)


def read_available(fd, timeout):
    if not select.select([fd], [], [], timeout)[0]:
        return None
    try:
        return os.read(fd, 65536)
    except OSError as error:
        if error.errno == errno.EIO and a.backend == 'native':
            return b''
        raise


try:
    for cpus in ([0] if a.backend == 'native' else a.cpus):
        for mode in a.modes:
            label = f'{mode}-{cpus}'
            command = [str(a.fixture.resolve()), mode, str(a.rounds)]
            if a.backend != 'native':
                empty = a.output / 'rootfs'
                (empty / 'tmp').mkdir(parents=True, exist_ok=True)
                command = [str(a.runner.resolve()), '--cpus', str(cpus), '--network', 'off',
                           '--metrics', '--pty', '--exec-file', f'/fixture={a.fixture.resolve()}']
                command += (['--rootfs', str(empty.resolve()), '--no-windows', '--timeout', '90']
                            if a.backend == 'hvf' else ['--memory', '--guest', '--accelerator', 'pure',
                                                        '--max-instructions', '3000000000'])
                command += ['/fixture', mode, str(a.rounds)]
            row = dict(cpus=cpus, mode=mode, command=command)
            report['cases'].append(row)
            master = slave = None
            with (a.output / (label + '.stderr')).open('wb') as errors:
                if a.backend == 'native':
                    master, slave = pty.openpty()
                    child = subprocess.Popen(command, stdin=slave, stdout=slave, stderr=errors,
                                             start_new_session=True)
                    os.close(slave)
                    slave = None
                    fd = master
                else:
                    child = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                             stderr=errors, env=dict(os.environ, TRAMES_MAC_BUNDLED='1'))
                    fd = child.stdout.fileno()
                began = time.monotonic()
                data = bytearray()
                try:
                    if mode == 'duplex':
                        deadline = began + 30
                        while b'READY\r\n' not in data and b'READY\n' not in data:
                            assert time.monotonic() < deadline, (label, 'READY timeout')
                            part = read_available(fd, .1)
                            assert part != b'', (label, 'exited before READY', child.poll())
                            if part:
                                data.extend(part)
                        # Keep stdout backpressured longer than the guest input
                        # deadline. Input must progress before we drain output.
                        time.sleep(.2)
                        os.write(master if master is not None else child.stdin.fileno(), b'x\n')
                        time.sleep(2.8)
                    deadline = began + 90
                    while True:
                        assert time.monotonic() < deadline, (label, 'output timeout')
                        part = read_available(fd, .1)
                        if part == b'':
                            break
                        if part:
                            data.extend(part)
                            assert len(data) < 9 * 1024 * 1024, (label, 'unexpected excess output')
                    row['code'] = child.wait(timeout=10)
                    row['wall_seconds'] = time.monotonic() - began
                finally:
                    if child.poll() is None:
                        child.kill()
                        child.wait()
                    if master is not None:
                        os.close(master)
                    if child.stdin:
                        child.stdin.close()
                    if child.stdout:
                        child.stdout.close()
                    (a.output / (label + '.stdout')).write_bytes(data)
            if a.baseline and row['code'] == 70 and mode == 'duplex':
                row['blocked_input'] = True
                print(label, 'BASELINE blocked input', flush=True)
                continue
            assert row['code'] == 0, row
            measured = json.loads(data.decode().strip().splitlines()[-1])
            row['measurement'] = measured
            assert measured['mode'] == mode and measured['failure'] == 0, row
            if mode == 'duplex':
                assert measured['input_ns'] < 2_000_000_000, row
                assert measured['bytes'] == 8 * 1024 * 1024
                assert data.count(b'Z') == measured['bytes'], (label, len(data))
            if a.backend != 'native':
                errors = (a.output / (label + '.stderr')).read_text()
                prefix = 'trames mac execution paths: ' if a.backend == 'hvf' else 'trames metrics: '
                metrics = json.loads(next(line[len(prefix):] for line in errors.splitlines()
                                          if line.startswith(prefix)))
                row['metrics'] = metrics
                assert metrics['service_requests'] == metrics['serviced_syscalls'] == 0, row
                if a.backend == 'hvf':
                    prefix = 'trames mac cohort-return syscalls: '
                    found = next(line[len(prefix):] for line in errors.splitlines()
                                 if line.startswith(prefix))
                    returned = ast.literal_eval(found)
                    row['cohort_return_syscalls'] = returned
                    if not a.baseline:
                        assert not any(k in (29, 63, 64) for k, _ in returned), row
            print(label, 'PASS', flush=True)
    report['passed'] = True
finally:
    (a.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
