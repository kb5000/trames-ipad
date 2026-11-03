#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare worker descriptor lifecycles with previous Trames and native APIs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
for name in ('baseline', 'candidate', 'native', 'fixture', 'rootfs', 'output'):
    p.add_argument('--' + name, type=Path)
p.add_argument('--native-only', action='store_true')
p.add_argument('--kind', choices=('unix-owning-rights','unix-owning-raw','unix-owning-datagram','netlink-publish','netlink-bytes','netlink-vector','netlink-message','netlink-poll','udp-raw','udp-vector','tcp-raw','tcp-vector','udp-message','tcp-message','udp-batch','tcp-batch','pipe', 'eventfd', 'timerfd', 'inotify', 'tcp', 'tcp-session', 'tcp-blocking', 'tcp-state', 'tcp-epoll', 'tcp-poll', 'epoll', 'inotify-watch', 'unix-rights', 'unix-create', 'unix-pair', 'file-positioned', 'device-null', 'device-zero', 'device-random', 'device-urandom', 'device-full', 'unix-batch', 'unix-packet', 'unix-packet-vector', 'unix-datagram', 'unix-sendto', 'unix-datagram-rights', 'unix-datagram-owner', 'unix-session', 'unix-abstract', 'signalfd', 'signal-rights', 'pty', 'pty-rights', 'pty-peer', 'pty-create'), required=True)
p.add_argument('--repeats', type=int, default=3)
p.add_argument('--operations', type=int, default=10000)
p.add_argument('--cpus', type=int, default=12)
a = p.parse_args()
for name in (('native', 'output') if a.native_only else
             ('baseline', 'candidate', 'fixture', 'rootfs', 'output')):
    if getattr(a, name) is None:
        p.error('--' + name + ' is required')
if a.native and platform.system() == 'Darwin' and a.kind not in ('unix-owning-rights','unix-owning-datagram','udp-raw','udp-vector','tcp-raw','tcp-vector','udp-message','tcp-message','udp-batch','tcp-batch','file-positioned', 'device-null', 'device-zero', 'pipe', 'tcp', 'tcp-session', 'tcp-blocking', 'tcp-state', 'tcp-epoll', 'tcp-poll', 'epoll', 'inotify-watch', 'unix-rights', 'unix-create', 'unix-pair', 'unix-batch', 'unix-packet', 'unix-packet-vector', 'unix-datagram', 'unix-sendto', 'unix-datagram-rights', 'unix-datagram-owner', 'unix-session'):
    p.error('this Linux ABI fixture has no native macOS reference; omit --native')
assert a.repeats > 0 and 0 < a.operations <= 1000000 and a.cpus > 0
a.output = a.output.resolve()
a.output.mkdir(parents=True, exist_ok=False)
report = dict(passed=False, kind=a.kind, processes=4, operations_each=a.operations,
              repeats=a.repeats, cpus=a.cpus, records=[], sha256={
                  key: hashlib.sha256(getattr(a, key).read_bytes()).hexdigest()
                  for key in ('baseline', 'candidate', 'native', 'fixture', 'rootfs')
                  if getattr(a, key) is not None}, native_environment=dict(
                      system=platform.system(), release=platform.release(),
                      architecture=platform.machine(), logical_cpus=os.cpu_count()))
env = dict(os.environ, TRAMES_MAC_BUNDLED='1')


def save():
    (a.output / 'results.json').write_text(json.dumps(report, indent=2) + '\n')


def parse(stdout):
    children = [json.loads(line) for line in stdout.splitlines()]
    assert len(children) == 4 and all(c['kind'] == a.kind and c['operations'] == a.operations
                                    and c['checksum'] == a.operations * (a.operations + 1) // 2
                                    for c in children)
    return children


def options(runner):
    return [str(runner.resolve()), '--no-pty', '--no-windows', '--network', 'loopback' if a.kind.startswith(('tcp','udp','netlink')) else 'off',
            '--cpus', str(a.cpus), '--memory-mib', 'auto', '--timeout', '60',
            '--storage', str(a.output / 'store')]


def guest(variant, repeat):
    prefix = 'TRAMES_BENCH_PTMX=/bench-pts/ptmx ' if a.kind == 'pty-create' else ''
    jobs = [f'{prefix}/bench {a.kind} {a.operations} & p{i}=$!' for i in range(4)]
    command = '; '.join(jobs + [f'wait $p{i} || exit $?' for i in range(4)])
    if a.kind == 'pty-create':
        command = ('mkdir -p /bench-pts && mount -t devpts -o newinstance,ptmxmode=0666,mode=0620,max=16 '
                   'devpts /bench-pts || exit $?; ' + command)
    args = options(a.baseline if variant == 'before' else a.candidate)
    if repeat == 0:
        args.append('--metrics')
    began = time.monotonic()
    result = subprocess.run(args + ['/bin/sh', '-c', command], env=env,
                            capture_output=True, timeout=70)
    name = f'{variant}-{repeat}'
    (a.output / (name + '.stdout')).write_bytes(result.stdout)
    (a.output / (name + '.stderr')).write_bytes(result.stderr)
    row = dict(variant=variant, repeat=repeat,
               wall_seconds=time.monotonic() - began, exit=result.returncode)
    report['records'].append(row)
    save()
    assert result.returncode == 0, name
    row['children'] = parse(result.stdout)
    if repeat == 0:
        prefix = b'trames mac execution paths: '
        row['paths'] = next(json.loads(line[len(prefix):]) for line in result.stderr.splitlines()
                            if line.startswith(prefix))
    save()


try:
    if not a.native_only:
        setup = subprocess.run(options(a.candidate) + [
            '--rootfs', str(a.rootfs.resolve()), '--exec-file', '/bench=' + str(a.fixture.resolve()),
            '/bin/true'], capture_output=True, env=env, timeout=120)
        (a.output / 'setup.stderr').write_bytes(setup.stderr)
        assert setup.returncode == 0, setup.stderr
        for variant in ('before', 'after'):
            guest(variant, 0)
        for repeat in range(1, a.repeats + 1):
            for variant in (('before', 'after') if repeat % 2 else ('after', 'before')):
                guest(variant, repeat)
    if a.native:
        for repeat in range(1, a.repeats + 1):
            began = time.monotonic()
            children = [subprocess.Popen([str(a.native.resolve()), a.kind, str(a.operations)],
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                        for _ in range(4)]
            rows = []
            for child in children:
                stdout, stderr = child.communicate(timeout=30)
                assert child.returncode == 0, stderr
                rows.append(stdout)
            report['records'].append(dict(variant='native', repeat=repeat,
                                          wall_seconds=time.monotonic() - began,
                                          children=parse(b''.join(rows)), exit=0))
    summary = {}
    for variant in (('native',) if a.native_only else
                    (('before', 'after', 'native') if a.native else ('before', 'after'))):
        rows = [r for r in report['records'] if r['variant'] == variant and r['repeat'] > 0]
        value = dict(loop_max_median_seconds=statistics.median(
            max(c['seconds'] for c in r['children']) for r in rows),
            process_wall_median_seconds=statistics.median(r['wall_seconds'] for r in rows))
        if variant != 'native':
            value['profile'] = next(r['paths'] for r in report['records']
                                    if r['variant'] == variant and r['repeat'] == 0)
        summary[variant] = value
    report.update(passed=True, summary=summary)
    print(json.dumps(summary, indent=2))
finally:
    save()
