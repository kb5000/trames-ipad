#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Collect equal-barrier append latency, process I/O and abrupt-exit recovery.

Run only in the unified validation phase. Every sample uses a fresh owned copy
of one frozen private store. Commands are argv arrays, never shell snippets.
Only the newly launched process group is killed. No production environment is
opened. Host process I/O is a sampled lower bound and excludes child processes;
it is not a block-device/power-loss measurement. Real package/container workloads
remain separate workload rows and cannot be inferred from append throughput.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import select
import shutil
import signal
import subprocess
import time

from process_accounting import Process, ProcessGone


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as source:
        while block := source.read(1 << 20):
            digest.update(block)
    return digest.hexdigest()


def kill_owned(process):
    # Retain the original Popen identity through wait; no name-based killing.
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    process.wait(timeout=10)


def command(template, substitutions):
    return [argument.format_map(substitutions) for argument in template]


class Batch:
    def __init__(self, args):
        self.args = args
        self.plan = json.loads(args.plan.read_text())
        self.output = args.output.resolve()
        self.source = Path(self.plan['source_store']).resolve(strict=True)
        if self.source == self.output or self.source in self.output.parents:
            raise ValueError('output must be outside the immutable source store')
        if not (self.source / 'manifest').is_file():
            raise ValueError('source_store must be a private store with its original manifest')
        self.inputs = {}
        self.store_bytes = 0
        for entry in sorted(self.source.rglob('*')):
            if entry.is_symlink():
                raise ValueError('private source store must not contain native symlinks')
            if entry.is_file():
                self.inputs[str(entry)] = sha(entry)
                self.store_bytes += entry.stat().st_size
        for entry in self.plan['inputs']:
            path = Path(entry).resolve(strict=True)
            self.inputs[str(path)] = sha(path)
        self.candidates = self.plan['candidates']
        names = [candidate['name'] for candidate in self.candidates]
        if not names or len(names) != len(set(names)) or any(not name.replace('-', '').isalnum() for name in names):
            raise ValueError('candidate names must be unique path-safe identifiers')
        contracts = {candidate['barrier_contract'] for candidate in self.candidates}
        if len(contracts) != 1 or not next(iter(contracts)):
            raise ValueError('all candidates must declare the same verified durability contract')
        for candidate in self.candidates:
            for phase in ('run', 'verify'):
                argv = candidate[phase]
                if not argv or any(not isinstance(part, str) for part in argv):
                    raise ValueError('run/verify must be nonempty argv arrays')
                executable = Path(shutil.which(argv[0]) or argv[0]).resolve(strict=True)
                self.inputs[str(executable)] = sha(executable)
                argv[0] = str(executable)
        if (not self.plan.get('writers') or any(type(n) is not int or not 1 <= n <= 16 for n in self.plan['writers'])
                or type(self.plan['rounds']) is not int or not 2 <= self.plan['rounds'] <= 100000
                or type(self.plan['repetitions']) is not int or not 1 <= self.plan['repetitions'] <= 20
                or not self.plan.get('sync') or any(mode not in ('full', 'data') for mode in self.plan['sync'])
                or not 0 < self.plan['timeout_seconds'] <= 3600):
            raise ValueError('invalid bounded workload plan')
        self.output.mkdir(parents=True, exist_ok=False)
        total_seconds = self.plan['total_timeout_seconds']
        if type(total_seconds) not in (int, float) or not 0 < total_seconds <= 7200:
            raise ValueError('pin a total batch deadline of at most two hours')
        self.deadline = time.monotonic() + total_seconds
        shutil.copyfile(args.plan, self.output / 'input-plan.json')
        self.report = dict(state='running', scope=__doc__, platform=platform.platform(),
                           architecture=platform.machine(), inputs=self.inputs, plan=self.plan, rows=[])
        self.checkpoint()

    def checkpoint(self):
        temporary = self.output / 'storage.json.tmp'
        temporary.write_text(json.dumps(self.report, indent=2) + '\n')
        temporary.replace(self.output / 'storage.json')

    def sample(self, candidate, writers, sync, repetition, crash):
        if time.monotonic() >= self.deadline:
            raise TimeoutError('total batch deadline; no new runtime launched')
        name = f'{candidate["name"]}-{writers}-{sync}-{repetition}-' + ('crash' if crash else 'complete')
        directory = self.output / name
        directory.mkdir()
        store = directory / 'store'
        reserve = self.plan.get('minimum_free_bytes', 2 << 30)
        if shutil.disk_usage(directory).free < self.store_bytes * 2 + reserve:
            raise RuntimeError('insufficient free space for the next owned store and recovery reserve')
        shutil.copytree(self.source, store)
        substitutions = dict(store=str(store), workdir=str(directory), writers=str(writers),
                             rounds=str(self.plan['rounds']), sync=sync,
                             guest_dir='/tmp/trames-durability-' + name)
        argv = command(candidate['run'], substitutions)
        row = dict(name=name, candidate=candidate['name'], writers=writers, sync=sync,
                   repetition=repetition, crash=crash, argv=argv, state='running',
                   acknowledged=[0] * writers, io_scope='sampled named process only; lower bound')
        self.report['rows'].append(row); self.checkpoint()
        latency = []
        began = time.monotonic()
        process = None
        try:
            with (directory / 'stdout.log').open('wb') as stdout, \
                    (directory / 'stderr.log').open('wb') as stderr, \
                    (directory / 'acknowledged.jsonl').open('w') as ledger:
                process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=stderr, start_new_session=True)
                row['host_pid'] = process.pid
                try:
                    accounting = Process(process.pid)
                    row['io_initial'] = accounting.first
                except ProcessGone:
                    accounting = None
                partial = bytearray()
                done = False
                killed = False
                ack_count = 0
                # Stop after a known observed ACK, well before all writers finish.
                crash_after = max(1, writers * self.plan['rounds'] // 2)
                while True:
                    if time.monotonic() - began >= self.plan['timeout_seconds'] or time.monotonic() >= self.deadline:
                        raise TimeoutError(name + ': run deadline')
                    if accounting:
                        try:
                            row['io_last_observed'] = accounting.snapshot()
                        except ProcessGone:
                            accounting = None
                    readable, _, _ = select.select([process.stdout], [], [], .05)
                    if not readable:
                        continue
                    block = os.read(process.stdout.fileno(), 65536)
                    if not block:
                        break
                    stdout.write(block); stdout.flush(); partial.extend(block)
                    if len(partial) > 1 << 20:
                        raise RuntimeError('unbounded line from original command')
                    while b'\n' in partial:
                        raw, _, remaining = partial.partition(b'\n'); partial = bytearray(remaining)
                        text = raw.rstrip(b'\r')
                        if text == b'STORAGE-DURABILITY-DONE':
                            done = True
                        if not text.startswith(b'{'):
                            continue
                        try:
                            ack = json.loads(text)
                        except (ValueError, UnicodeDecodeError):
                            continue
                        if not isinstance(ack, dict) or 'writer' not in ack:
                            continue
                        writer = ack['writer']
                        if (type(writer) is not int or not 0 <= writer < writers or
                                ack.get('sequence') != row['acknowledged'][writer] + 1 or
                                ack.get('sequence', 0) > self.plan['rounds'] or
                                ack.get('sync') != sync or ack.get('bytes') != 4096 or
                                type(ack.get('latency_ns')) is not int or ack['latency_ns'] < 0):
                            raise RuntimeError('original fixture ACK identity/order differs: ' + repr(ack))
                        # The independent host ledger survives termination of the
                        # owned runtime group; acknowledge only fully parsed lines.
                        ledger.write(json.dumps(ack) + '\n'); ledger.flush()
                        row['acknowledged'][writer] = ack['sequence']
                        ack_count += 1; latency.append(ack['latency_ns'])
                        if crash and ack_count == crash_after:
                            row['kill_seconds'] = time.monotonic() - began
                            self.checkpoint()
                            kill_owned(process); killed = True
                            break
                    if killed:
                        # The buffered but unobserved suffix is not an ACK in the
                        # recovery contract. Preserve it only as raw stdout.
                        break
                code = process.wait(timeout=10)
                row.update(exit=code, seconds=time.monotonic() - began, killed_after_ack=killed)
                if crash:
                    if not killed or code != -signal.SIGKILL:
                        raise RuntimeError('the original runtime was not killed at the selected ACK')
                elif code != 0 or not done or row['acknowledged'] != [self.plan['rounds']] * writers:
                    raise RuntimeError('fixture did not complete all writers/barriers')
            if latency:
                ordered = sorted(latency)
                row.update(acknowledgements=len(ordered), p50_ns=ordered[math.ceil(len(ordered) * .5) - 1],
                           p99_ns=ordered[math.ceil(len(ordered) * .99) - 1], maximum_ns=ordered[-1],
                           acknowledged_bytes=len(ordered) * 4096)
            first, last = row.get('io_initial'), row.get('io_last_observed')
            if first and last:
                row['host_io_delta_lower_bound'] = {
                    metric: max(0, last[metric] - first[metric]) if last[metric] is not None and first[metric] is not None else None
                    for metric in ('read_bytes', 'written_bytes')}
            # Each verify invocation cold-opens the same store in a new original
            # runtime process. No in-process cache can establish recovery success.
            for writer, count in enumerate(row['acknowledged']):
                if not count:
                    continue
                verify = command(candidate['verify'], dict(substitutions, writer=str(writer), acknowledged=str(count)))
                with (directory / f'verify-{writer}.stdout').open('wb') as out, \
                        (directory / f'verify-{writer}.stderr').open('wb') as err:
                    recovery = subprocess.Popen(verify, stdout=out, stderr=err, start_new_session=True)
                    try:
                        status = recovery.wait(timeout=max(.01, min(self.plan['timeout_seconds'], self.deadline - time.monotonic())))
                    except BaseException:
                        kill_owned(recovery)
                        raise
                text = (directory / f'verify-{writer}.stdout').read_text(errors='replace')
                if status != 0 or 'STORAGE-DURABILITY-VERIFIED' not in text:
                    raise RuntimeError(f'cold recovery failed for writer {writer}, ACK prefix {count}')
            row['state'] = 'complete'
            # Only this invocation's known disposable store is removed. Keep all
            # logs, frozen inputs and failed stores for diagnosis.
            if not self.plan.get('keep_successful_stores', False):
                shutil.rmtree(store)
                row['successful_store_removed'] = True
        except BaseException as error:
            row.update(state='failed', error=repr(error))
            raise
        finally:
            if process is not None and process.poll() is None:
                kill_owned(process)
            self.checkpoint()

    def run(self):
        failures = []
        try:
            for repetition in range(self.plan['repetitions']):
                # Alternate complete candidate order; no candidate is always warm.
                candidates = self.candidates if repetition % 2 == 0 else list(reversed(self.candidates))
                for writers in self.plan['writers']:
                    for sync in self.plan['sync']:
                        for candidate in candidates:
                            for crash in (False, True):
                                if time.monotonic() >= self.deadline:
                                    raise TimeoutError('total batch deadline; remaining rows were not run')
                                try:
                                    self.sample(candidate, writers, sync, repetition, crash)
                                except Exception as error:
                                    failures.append(dict(candidate=candidate['name'], writers=writers, sync=sync,
                                                         repetition=repetition, crash=crash, error=repr(error)))
                                    self.report['failures'] = failures
                                    self.checkpoint()
            self.report['state'] = 'failed' if failures else 'complete'
            if failures:
                raise RuntimeError(f'{len(failures)} workload rows failed; see retained records')
        except BaseException as error:
            self.report.update(state='failed', error=repr(error))
            raise
        finally:
            self.checkpoint()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    Batch(parser.parse_args()).run()
