#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fixed cold-session/idle/stop accounting through the production embedding ABI.

The measurement process embeds runtime and guest. A separately named manager is
reported separately, never subtracted as a guessed baseline. This host harness
does not establish a physical iPad or clean-install App baseline. It copies an
existing disposable store or installs a pinned archive into a fresh owned store.
"""
import argparse
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import time
import tomllib

from process_accounting import Process


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as source:
        while block := source.read(1 << 20):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--options', type=Path, required=True, help='frozen FFI options, with environment_config')
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--store', type=Path, help='read-only source; copied without hard links')
    source.add_argument('--rootfs', type=Path, help='cold import, not an installed-store baseline')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manager-pid', type=int, help='read-only separate manager accounting')
    parser.add_argument('--idle-seconds', type=float, default=30)
    parser.add_argument('--stopped-seconds', type=float, default=15)
    parser.add_argument('--interval', type=float, default=.1)
    parser.add_argument('--timeout', type=float, default=300)
    args = parser.parse_args()
    if not (.02 <= args.interval <= 5 and args.idle_seconds >= 1 and
            args.stopped_seconds >= 1 and args.timeout > args.idle_seconds + args.stopped_seconds):
        raise ValueError('invalid bounded sampling schedule')
    library = args.library.resolve(strict=True)
    options = json.loads(args.options.read_text())
    forbidden = {'store', 'rootfs', 'store_owner_lock', 'command', 'initially_paused', 'debian_setup',
                 'environment_grants'}
    if forbidden.intersection(options):
        raise ValueError('harness owns these options: ' + ', '.join(sorted(forbidden.intersection(options))))
    if options.get('target', 'linux') != 'linux':
        raise ValueError('this shell scenario measures Linux; Android requires its real boot scenario')
    config = Path(options['environment_config']).resolve(strict=True)
    declaration = tomllib.loads(config.read_text())
    cpus = declaration.get('resources', {}).get('cpus')
    if type(cpus) is not int or cpus < 1:
        raise ValueError('pin a numeric resources.cpus in the environment declaration')
    # Freeze all declared native inputs before loading code or sampling. Directory
    # bundles are hashed file-by-file; no host cache dropping or RAM reservation.
    identities = {str(library): sha(library), str(args.options.resolve()): sha(args.options)}
    def freeze(path):
        path = path.resolve(strict=True)
        if path.is_dir():
            for entry in sorted(path.rglob('*')):
                if entry.is_file():
                    identities[str(entry)] = sha(entry)
        else:
            identities[str(path)] = sha(path)
        return path
    for key in ('musl_libc', 'bionic_bundle', 'glibc_bundle', 'vulkan_icd', 'video_driver',
                'environment_config', 'environment_grants', 'xwayland_launcher'):
        if options.get(key):
            options[key] = str(freeze(Path(options[key])))
    original = freeze(args.store if args.store else args.rootfs)
    if args.store and any(path.is_symlink() for path in original.rglob('*')):
        raise ValueError('a private source store must contain regular objects, not native symlinks')
    output = args.output.resolve()
    if original == output or original in output.parents:
        raise ValueError('output must be outside the source store/archive')
    output.mkdir(parents=True, exist_ok=False)
    owned = output / 'store'
    if args.store:
        shutil.copytree(original, owned)
    else:
        options['rootfs'] = str(original)
        options['rootfs_sha256'] = sha(original)
    shutil.copyfile(config, output / 'environment.toml')
    options['environment_config'] = str(output / 'environment.toml')
    options['store'] = str(owned)
    options['initially_paused'] = False
    # Print one ready marker after exec and real /proc queries, then block in the
    # original shell's read. This adds no periodic guest task during idle sampling.
    script = "printf '\\nMEMORY-GUEST-STATUS-BEGIN\\n'; cat /proc/$$/status; " \
             "printf '\\nMEMORY-GUEST-CPU-BEGIN\\n'; cat /proc/cpuinfo; " \
             "printf '\\nMEMORY-IDLE-READY\\n'; read memory_exit"
    options['command'] = ['/bin/sh', '-c', script]
    (output / 'options.json').write_text(json.dumps(options, indent=2) + '\n')
    report = dict(state='running', scope=__doc__, platform=platform.platform(), architecture=platform.machine(),
                  pid=os.getpid(), cpu_count=cpus, source_kind='installed-store-copy' if args.store else 'archive-import',
                  inputs=identities, options=options, interval_seconds=args.interval,
                  samples_file='samples.jsonl', phases={}, physical_ipad=False,
                  peak_kind='sampled, not a kernel high-water mark')
    def checkpoint():
        temporary = output / 'memory.json.tmp'
        temporary.write_text(json.dumps(report, indent=2) + '\n')
        temporary.replace(output / 'memory.json')
    checkpoint()
    manager = Process(args.manager_pid) if args.manager_pid else None
    if args.manager_pid == os.getpid():
        raise ValueError('manager and embedded runtime must be separately identified processes')
    api = C.CDLL(str(library))
    api.trames_session_open.argtypes = [C.c_char_p, C.c_uint32]
    api.trames_session_open.restype = C.c_uint64
    api.trames_session_close.argtypes = [C.c_uint64]
    for name in ('trames_session_output', 'trames_session_status'):
        getattr(api, name).argtypes = [C.c_uint64, C.c_void_p, C.c_uint32]
    api.trames_process_memory.argtypes = [C.c_void_p, C.c_uint32]
    api.trames_last_error.argtypes = [C.c_void_p, C.c_uint32]
    handle = 0
    began = time.monotonic()
    terminal = bytearray()
    terminal_log = (output / 'terminal.log').open('wb')
    sample_log = (output / 'samples.jsonl').open('w')
    def error():
        buffer = C.create_string_buffer(8192)
        count = api.trames_last_error(buffer, len(buffer))
        return buffer.raw[:max(0, count)].decode(errors='replace')
    def snapshot(phase):
        buffer = C.create_string_buffer(4096)
        count = api.trames_process_memory(buffer, len(buffer))
        if count < 0:
            raise RuntimeError(error())
        value = json.loads(buffer.raw[:count])
        row = dict(seconds=time.monotonic() - began, phase=phase, embedded_process=value)
        if manager:
            row['manager'] = manager.snapshot()
        # Stream raw samples instead of retaining them in the process whose RSS
        # is being measured. Summary calculation happens after sampling ends.
        sample_log.write(json.dumps(row) + '\n'); sample_log.flush()
        return value
    def sample_period(phase, seconds):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            if time.monotonic() - began >= args.timeout:
                raise TimeoutError('total measurement deadline')
            value = snapshot(phase)
            if phase == 'stopped' and value['runtime_workers'] != 0:
                raise RuntimeError('runtime restarted during stopped accounting')
            checkpoint()
            time.sleep(min(args.interval, max(0, deadline - time.monotonic())))
    try:
        if snapshot('loaded-before-session')['runtime_workers'] != 0:
            raise RuntimeError('the isolated measurement process already owns sessions')
        raw = json.dumps(options).encode()
        handle = api.trames_session_open(raw, len(raw))
        if not handle:
            raise RuntimeError(error())
        report['phases']['open'] = time.monotonic() - began
        while b'\r\nMEMORY-IDLE-READY\r\n' not in terminal and b'\nMEMORY-IDLE-READY\n' not in terminal:
            if time.monotonic() - began >= args.timeout - args.idle_seconds - args.stopped_seconds:
                raise TimeoutError('guest startup deadline')
            snapshot('startup')
            buffer = C.create_string_buffer(65536)
            count = api.trames_session_output(handle, buffer, len(buffer))
            if count < 0:
                raise RuntimeError(error())
            terminal.extend(buffer.raw[:count]); terminal_log.write(buffer.raw[:count]); terminal_log.flush()
            if len(terminal) > 2 << 20:
                raise RuntimeError('unexpected unbounded startup output')
            count = api.trames_session_status(handle, buffer, len(buffer))
            if count < 0:
                raise RuntimeError(error())
            status = json.loads(buffer.raw[:count])
            if status.get('error') or status['phase'] in ('failed', 'exited'):
                raise RuntimeError('guest did not remain alive: ' + repr(status))
            checkpoint(); time.sleep(args.interval)
        report['phases']['idle'] = time.monotonic() - began
        actual_cpus = len(re.findall(rb'^processor\s*:', terminal, re.M))
        if actual_cpus != cpus:
            raise RuntimeError(f'guest reported {actual_cpus} CPUs, declared {cpus}')
        report['observed_guest_cpus'] = actual_cpus
        terminal.clear()
        sample_period('idle', args.idle_seconds)
        if api.trames_session_close(handle) < 0:
            raise RuntimeError(error())
        handle = 0
        report['phases']['close-requested'] = time.monotonic() - began
        while snapshot('closing')['runtime_workers']:
            if time.monotonic() - began >= args.timeout - args.stopped_seconds:
                raise TimeoutError('asynchronous worker retirement deadline')
            checkpoint(); time.sleep(args.interval)
        report['phases']['workers-retired'] = time.monotonic() - began
        sample_period('stopped', args.stopped_seconds)
        report['state'] = 'complete'
    except BaseException as exception:
        report.update(state='failed', error=repr(exception))
        raise
    finally:
        if handle:
            api.trames_session_close(handle)
        terminal_log.close()
        sample_log.close()
        summaries = {}
        with (output / 'samples.jsonl').open() as samples:
            for line in samples:
                row = json.loads(line)
                for component in ('embedded_process', 'manager'):
                    if component not in row:
                        continue
                    for metric in ('resident_bytes', 'physical_footprint_bytes'):
                        value = row[component][metric]
                        if value is None:
                            continue
                        key = row['phase'] + '/' + component + '/' + metric
                        summary = summaries.setdefault(key, dict(count=0, minimum=value, maximum=value, total=0))
                        summary.update(count=summary['count'] + 1, minimum=min(summary['minimum'], value),
                                       maximum=max(summary['maximum'], value), total=summary['total'] + value,
                                       last=value)
        for summary in summaries.values():
            summary['mean'] = summary.pop('total') / summary['count']
        report['summary'] = summaries
        checkpoint()


if __name__ == '__main__':
    main()
