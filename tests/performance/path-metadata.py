#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare fixed-work pathname metadata on two Mac runners and native macOS."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import statistics
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
for name in ('baseline', 'candidate', 'native', 'fixture', 'rootfs', 'output'):
    p.add_argument('--' + name, type=Path)
p.add_argument('--native-only', action='store_true', help='run the same native reference on Linux or macOS')
p.add_argument('--native-directory', type=Path, help='use a native filesystem directory separate from report output')
p.add_argument('--prepare-directory', action='store_true', help='prepare directory contents sequentially before timing')
p.add_argument('--repeats', type=int, default=3)
p.add_argument('--cpus', type=int, default=12)
p.add_argument('--operations', type=int, default=10000)
p.add_argument('--mode', choices=('stat', 'openclose', 'directory'), default='stat')
a = p.parse_args()
for name in (('native', 'output') if a.native_only else
             ('baseline', 'candidate', 'native', 'fixture', 'rootfs', 'output')):
    if getattr(a, name) is None:
        p.error('--' + name + ' is required')
assert a.repeats > 0 and 0 < a.operations <= 1000000
a.output = a.output.resolve()
a.output.mkdir(parents=True, exist_ok=False)
shared = a.output / 'shared'
shared.mkdir()
store = a.output / 'store'
native = a.native_directory.resolve() if a.native_directory else a.output / 'native'
native.mkdir()
env = dict(os.environ, TRAMES_MAC_BUNDLED='1')
report = dict(passed=False, processes=4, operations_each=a.operations,
              cpus=a.cpus, mode=a.mode, repeats=a.repeats, native_directory=str(native), prepared_directory_contents=a.prepare_directory, records=[], sha256={
                  key: hashlib.sha256(getattr(a, key).read_bytes()).hexdigest()
                  for key in ('baseline', 'candidate', 'native', 'fixture', 'rootfs')
                  if getattr(a, key) is not None}, native_environment=dict(
                      system=platform.system(), release=platform.release(),
                      architecture=platform.machine(), logical_cpus=os.cpu_count()))


def save():
    (a.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')


def options(runner):
    return [str(runner.resolve()), '--no-pty', '--no-windows', '--network', 'off',
            '--cpus', str(a.cpus), '--memory-mib', 'auto', '--timeout', '60',
            '--storage', str(store), '--mount', '/work=' + str(shared)]


def guest(kind, variant, repeat):
    directory = '/tmp' if kind == 'private' else '/work'
    jobs = [f'/bench {shlex.quote(directory + "/metadata-" + str(i))} {a.mode} {a.operations} & p{i}=$!'
            for i in range(4)]
    command = '; '.join(jobs + [f'wait $p{i} || exit $?' for i in range(4)])
    runner = a.baseline if variant == 'before' else a.candidate
    args = options(runner)
    if repeat == 0:
        args.append('--metrics')
    began = time.monotonic()
    result = subprocess.run(args + ['/bin/sh', '-c', command], env=env,
                            capture_output=True, timeout=70)
    name = f'{kind}-{variant}-{repeat}'
    (a.output / (name + '.stdout')).write_bytes(result.stdout)
    (a.output / (name + '.stderr')).write_bytes(result.stderr)
    row = dict(kind=kind, variant=variant, repeat=repeat,
               wall_seconds=time.monotonic() - began, exit=result.returncode)
    report['records'].append(row)
    save()
    assert result.returncode == 0, name
    row['children'] = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(row['children']) == 4 and all(c['operations'] == a.operations for c in row['children'])
    if a.mode == 'directory':
        assert all(c['checksum'] == a.operations * (a.operations + 1) // 2 for c in row['children'])
    if repeat == 0:
        prefix = b'trames mac execution paths: '
        row['paths'] = next(json.loads(line[len(prefix):]) for line in result.stderr.splitlines()
                            if line.startswith(prefix))
    save()


try:
    # Prepare one disposable store once. Both versions then use the same image,
    # fixture, file contents and mount; import time is outside every timed sample.
    if not a.native_only:
        setup = subprocess.run(options(a.candidate) + [
            '--rootfs', str(a.rootfs.resolve()), '--exec-file', '/bench=' + str(a.fixture.resolve()),
            '/bin/true'], capture_output=True, env=env, timeout=120)
        (a.output / 'setup.stderr').write_bytes(setup.stderr)
        assert setup.returncode == 0, setup.stderr
        if a.mode == 'directory' and a.prepare_directory:
            commands = [f'/bench {shlex.quote(directory + "/metadata-" + str(i))} directory 1'
                        for directory in ('/tmp', '/work') for i in range(4)]
            prepared = subprocess.run(options(a.candidate) + ['/bin/sh', '-c', ' && '.join(commands)],
                                      capture_output=True, env=env, timeout=120)
            (a.output / 'prepare.stdout').write_bytes(prepared.stdout)
            (a.output / 'prepare.stderr').write_bytes(prepared.stderr)
            assert prepared.returncode == 0, prepared.stderr
        for kind in ('private', 'shared'):
            # Profile separately; no clock/counter instrumentation in timed samples.
            for variant in ('before', 'after'):
                guest(kind, variant, 0)
            for repeat in range(1, a.repeats + 1):
                for variant in (('before', 'after') if repeat % 2 else ('after', 'before')):
                    guest(kind, variant, repeat)
    for repeat in range(1, a.repeats + 1):
        began = time.monotonic()
        children = [subprocess.Popen([str(a.native.resolve()), str(native / f'metadata-{i}'),
                                      a.mode, str(a.operations)], stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE) for i in range(4)]
        rows = []
        for child in children:
            stdout, stderr = child.communicate(timeout=30)
            assert child.returncode == 0, stderr
            row = json.loads(stdout)
            assert row['operations'] == a.operations
            if a.mode == 'directory':
                assert row['checksum'] == a.operations * (a.operations + 1) // 2
            rows.append(row)
        report['records'].append(dict(kind='native', variant='native', repeat=repeat,
                                      wall_seconds=time.monotonic() - began, children=rows, exit=0))
    summary = {}
    for kind in (('native',) if a.native_only else ('private', 'shared', 'native')):
        summary[kind] = {}
        for variant in (('native',) if kind == 'native' else ('before', 'after')):
            rows = [r for r in report['records'] if r['kind'] == kind
                    and r['variant'] == variant and r['repeat'] > 0]
            row = dict(loop_max_median_seconds=statistics.median(
                max(c['seconds'] for c in r['children']) for r in rows),
                process_wall_median_seconds=statistics.median(r['wall_seconds'] for r in rows))
            if kind != 'native':
                row['profile'] = next(r['paths'] for r in report['records']
                                      if r['kind'] == kind and r['variant'] == variant and r['repeat'] == 0)
            summary[kind][variant] = row
    report.update(passed=True, summary=summary)
    print(json.dumps(summary, indent=2))
finally:
    save()
