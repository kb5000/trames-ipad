#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Actual Mac product CLI/HVF task-limit acceptance in disposable registries.

Supply the signed bundle's Contents/MacOS/trames-mac-run and Linux ARM64 ELFs
compiled from task-limit.c and task-capacity.c. Existing user stores are unused.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', type=Path, required=True)
    parser.add_argument('--rootfs', type=Path, required=True)
    parser.add_argument('--fixtures', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    runner = str(args.runner.resolve())
    digest = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    report = dict(passed=False, runner_sha256=digest(args.runner),
                  rootfs_sha256=digest(args.rootfs),
                  fixtures={n: digest(args.fixtures / n) for n in ['task-limit', 'task-capacity']},
                  rows=[], scenarios=[])
    archive = args.output / 'acceptance-rootfs.tar.gz'
    with tarfile.open(args.rootfs, 'r:gz') as source, tarfile.open(archive, 'w:gz') as target:
        for entry in source:
            target.addfile(entry, source.extractfile(entry) if entry.isfile() else None)
        directory = tarfile.TarInfo('opt/trames-task')
        directory.type = tarfile.DIRTYPE
        directory.mode = 0o755
        target.addfile(directory)
        for name in ['task-limit', 'task-capacity']:
            entry = tarfile.TarInfo('opt/trames-task/' + name)
            entry.mode = 0o755
            entry.size = (args.fixtures / name).stat().st_size
            with (args.fixtures / name).open('rb') as source:
                target.addfile(entry, source)
    report['acceptance_rootfs_sha256'] = digest(archive)

    def save():
        (args.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')

    for cpus in [1, 4]:
        registry = args.output / ('registry-' + str(cpus))
        environment = dict(os.environ, TRAMES_HOME=str(registry.resolve()))

        def run(*arguments, ok=True, timeout=90):
            began = time.monotonic()
            row = dict(cpus=cpus, arguments=list(arguments))
            try:
                result = subprocess.run([runner, *map(str, arguments)], env=environment,
                                        capture_output=True, text=True, timeout=timeout)
                row.update(exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr)
            except subprocess.TimeoutExpired as error:
                row.update(exit_code=124, timeout=True,
                           stdout=(error.stdout or b'').decode(errors='replace'),
                           stderr=(error.stderr or b'').decode(errors='replace'))
            row['seconds'] = round(time.monotonic() - began, 3)
            report['rows'].append(row)
            save()
            print(json.dumps(row), flush=True)
            if ok is not None:
                assert (row['exit_code'] == 0) == ok, row
            return row

        def configure(name, *settings):
            return run('-e', name, 'config', 'set', *settings)

        def execute(name, *command, timeout=90):
            return run('-e', name, 'exec', '--', *command, timeout=timeout)['stdout']

        try:
            run('env', 'create', 'work', '--distro', 'alpine', '--rootfs', str(archive.resolve()))
            configure('work', 'resources.cpus', str(cpus), 'network.mode', 'none',
                      'components.vulkan', 'false', 'components.vaapi', 'false',
                      'display.windows', 'false')
            run('env', 'start', 'work')
            execute('work', '/bin/sh', '-ec', 'test "$(grep -c \'^processor\' /proc/cpuinfo)" -eq ' + str(cpus))
            assert 'TASK_LIMIT_BOOT:65536' in execute('work', '/opt/trames-task/task-limit', 'read', '65536')
            run('env', 'stop', 'work')
            configure('work', 'kernel.sysctl.kernel.threads-max', '64')
            assert run('-e', 'work', 'config', 'get', 'kernel.sysctl.kernel.threads-max')['stdout'].strip() == '64'
            path = Path(run('-e', 'work', 'config', 'path')['stdout'].strip())
            before = path.read_bytes()
            for value in ['0', '-1', '1.5', '1073741824']:
                run('-e', 'work', 'config', 'set', 'kernel.sysctl.kernel.threads-max', value, ok=False)
                assert path.read_bytes() == before
            run('-e', 'work', 'config', 'set', 'kernel.sysctl.kernel.unknown', '2', ok=False)
            assert path.read_bytes() == before
            run('env', 'start', 'work')
            text = execute('work', '/opt/trames-task/task-limit', 'live', '64')
            assert 'TASK_LIMIT_LIVE_OK:' in text and 'final=65' in text, text
            assert execute('work', '/bin/cat', '/proc/sys/kernel/threads-max').strip() == '65'
            assert run('-e', 'work', 'config', 'get', 'kernel.sysctl.kernel.threads-max')['stdout'].strip() == '64'
            assert path.read_bytes() == before
            run('env', 'stop', 'work')
            run('env', 'start', 'work')
            assert 'TASK_LIMIT_BOOT:64' in execute('work', '/opt/trames-task/task-limit', 'read', '64')
            run('env', 'stop', 'work')
            configure('work', 'kernel.sysctl.kernel.threads-max', '600')
            run('env', 'start', 'work')
            text = execute('work', '/opt/trames-task/task-capacity', '512', timeout=150)
            assert text.count('task-capacity-round:') == 4 and 'task-capacity-ok:' in text, text
            run('env', 'stop', 'work')
            # Low-level embedding limits are applied before the first task;
            # neither endpoint requires preallocating that many task objects.
            store = path.parent / 'store'
            for limit in [1, 1073741823]:
                row = run('debug', 'run', '--storage', str(store), '--cpus', str(cpus),
                          '--sysctl', 'kernel.threads-max=' + str(limit), '--no-windows',
                          '--no-pty', '--network', 'loopback', '--timeout', '30', '--',
                          '/opt/trames-task/task-limit', 'read', str(limit))
                assert 'TASK_LIMIT_BOOT:' + str(limit) in row['stdout']
            run('config', '--global', 'set', 'defaults.kernel.sysctl.kernel.threads-max', '42',
                'defaults.resources.cpus', str(cpus), 'defaults.network.mode', 'none',
                'defaults.components.vulkan', 'false', 'defaults.components.vaapi', 'false',
                'defaults.display.windows', 'false')
            assert run('config', '--global', 'get', 'defaults.kernel.sysctl.kernel.threads-max')['stdout'].strip() == '42'
            run('env', 'create', 'inherited', '--distro', 'alpine', '--rootfs', str(archive.resolve()))
            assert run('-e', 'inherited', 'config', 'get', 'kernel.sysctl.kernel.threads-max')['stdout'].strip() == '42'
            run('env', 'start', 'inherited')
            assert 'TASK_LIMIT_BOOT:42' in execute('inherited', '/opt/trames-task/task-limit', 'read', '42')
            run('env', 'stop', 'inherited')
            report['scenarios'].append(dict(cpus=cpus, passed=True))
        except Exception as error:
            report['scenarios'].append(dict(cpus=cpus, passed=False, error=str(error)))
        finally:
            for name in ['work', 'inherited']:
                run('env', 'stop', name, ok=None, timeout=30)
            save()
    report['passed'] = all(row['passed'] for row in report['scenarios'])
    save()
    if not report['passed']:
        raise SystemExit('product task-capacity acceptance failed; see results.json')


if __name__ == '__main__':
    main()
