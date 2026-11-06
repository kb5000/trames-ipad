#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Original task credentials and permission observers on native Linux, HVF and the shared interpreter."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--backend', choices=['native', 'hvf', 'interpreter'], required=True)
p.add_argument('--runner', type=Path)
p.add_argument('--fixture', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--cpus', type=int, nargs='+', default=[1, 2, 4, 12])
a = p.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
empty = a.output / 'empty-rootfs'
empty.mkdir(exist_ok=True)
digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
report = dict(backend=a.backend, fixture_sha256=digest(a.fixture), cases=[])
if a.backend != 'native':
    if a.runner is None:
        p.error('--runner is required for a runtime backend')
    report['runner_sha256'] = digest(a.runner)
for cpus in a.cpus:
    for filtered in [False, True]:
        if a.backend == 'native':
            command = [str(a.fixture.resolve())]
        else:
            command = [str(a.runner.resolve()), '--cpus', str(cpus), '--network', 'off',
                       '--exec-file', f'/worker-credentials={a.fixture.resolve()}', '--metrics']
            command += (['--rootfs', str(empty.resolve()), '--timeout', '60', '--no-pty', '--no-windows']
                        if a.backend == 'hvf' else ['--memory', '--guest'])
            command += ['/worker-credentials']
        command += [str(cpus), '16'] + (['filtered'] if filtered else [])
        result = subprocess.run(command, capture_output=True, timeout=70)
        name = f'{cpus}-' + ('filtered' if filtered else 'raw')
        (a.output / (name + '.log')).write_bytes(result.stdout + result.stderr)
        row = dict(cpus=cpus, filtered=filtered, returncode=result.returncode)
        report['cases'].append(row)
        (a.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
        assert result.returncode == 0, (name, result.stdout, result.stderr[:1500])
        value = next(json.loads(line) for line in result.stdout.decode().splitlines() if line.startswith('{'))
        assert value['status'] == 'pass' and value['threads'] == cpus and value['loops'] == 16
        print(json.dumps(row), flush=True)
