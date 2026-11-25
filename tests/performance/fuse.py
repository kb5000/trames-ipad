#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Matched release A/B FUSE measurements, with independently checked clients."""
import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import signal
import statistics
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
for name in ('baseline', 'candidate', 'rootfs', 'tools', 'client', 'workload', 'output'):
    p.add_argument('--' + name, type=Path, required=True)
p.add_argument('--cpus', nargs='+', type=int, default=[1, 2, 4])
p.add_argument('--profiles', nargs='+', choices=['lowlevel', 'writeback', 'highlevel'], default=['lowlevel', 'writeback', 'highlevel'])
p.add_argument('--repeats-per-block', type=int, default=2)
p.add_argument('--blocks', type=int, default=2)
p.add_argument('--min-free-gib', type=float, default=3)
p.add_argument('--ordinary-scale', type=int, default=1, help='Multiply the ordinary client fixed work; requires the matching client for values above one')
p.add_argument('--baseline-writeback', action='store_true', help='Require negotiated writeback in both artifacts')
a = p.parse_args()
assert a.blocks >= 2 and a.repeats_per_block >= 2 and all(n > 0 for n in a.cpus) and 1 <= a.ordinary_scale <= 100
a.output.mkdir(parents=True, exist_ok=False)
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
inputs = {n: str(getattr(a, n).resolve()) for n in ('baseline','candidate','rootfs','client','workload')}
inputs.update({n: str((a.tools/n).resolve()) for n in ('passthrough_ll','passthrough')})
assert sha(Path(inputs['baseline'])) != sha(Path(inputs['candidate'])), 'identical A/B artifacts'
report = dict(passed=False, profile='release', measured_daemon_debug=False, baseline_writeback=a.baseline_writeback, ordinary_scale=a.ordinary_scale,
              timing='client CLOCK_MONOTONIC nanoseconds; content verification included in reads',
              order='AB then BA per profile/CPU; separate warmup sample 0 in every process',
              environment=dict(system=platform.system(),release=platform.release(),machine=platform.machine(),logical_cpus=os.cpu_count()),
              inputs={n:dict(path=q,sha256=sha(Path(q))) for n,q in inputs.items()}, runs=[], rows=[], summary=[])
def save(): (a.output/'results.json').write_text(json.dumps(report,indent=2)+'\n')
def run(label, profile, cpus, block, diagnostic=False):
    prefix=f'{profile}-{cpus}-{block}-{label}' + ('-diagnostic' if diagnostic else '')
    store=a.output/(prefix+'-store')
    assert not store.exists()
    assert shutil.disk_usage(a.output).free > a.min_free_gib*1024**3, 'disk floor'
    command=[inputs[label],'--storage',str(store.resolve()),'--network','off','--virtual-network','2',
             '--cpus',str(cpus),'--rootfs',inputs['rootfs'],'--memory-budget','2GiB','--process-memory-budget','1GiB',
             '--max-instructions','20000000000']
    for name in ('passthrough_ll','passthrough'):
        command+=['--exec-file',f'/opt/fuse-tool/{name}={inputs[name]}']
    command+=['--exec-file',f'/opt/fuse-tool/client={inputs["client"]}',
              '--exec-file',f'/opt/fuse-tool/workload.sh={inputs["workload"]}']
    for name in ('fuse-daemon.stdout','fuse-daemon.stderr'):
        command+=['--export',f'/tmp/{name}={a.output.resolve()}/{prefix}-{name}']
    repeats=0 if diagnostic else a.repeats_per_block
    workers=1 if diagnostic else cpus
    command+=['--guest','--','/bin/sh','/opt/fuse-tool/workload.sh',profile,str(repeats),str(workers),str(int(diagnostic)),str(1 if diagnostic else a.ordinary_scale)]
    began=time.monotonic()
    load_before=os.getloadavg()
    with (a.output/(prefix+'.stdout')).open('w') as stdout,(a.output/(prefix+'.stderr')).open('w') as stderr:
        child=subprocess.Popen(command,stdout=stdout,stderr=stderr,start_new_session=True)
        reason=None
        try: child.wait(timeout=180)
        except subprocess.TimeoutExpired:
            reason='timeout'
            os.killpg(child.pid,signal.SIGTERM)
            try: child.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(child.pid,signal.SIGKILL)
                child.wait()
    text=(a.output/(prefix+'.stdout')).read_text()
    rows=[json.loads(line) for line in text.splitlines() if line.startswith('{')]
    complete=child.returncode==0 and reason is None and f'PERFORMANCE {profile} DONE' in text
    for sample in range(repeats+1):
        found=[r for r in rows if r['sample']==sample]
        complete=complete and collections.Counter(r['kind'] for r in found)==dict(transfer=1,ordinary=workers)
    for row in rows:
        complete=complete and all(isinstance(v,int) and v>0 for k,v in row.items() if k.endswith('_ns'))
        if row['kind']=='transfer': complete=complete and row['bytes']==8388608
        report['rows'].append(dict(row,label=label,profile=profile,cpus=cpus,block=block,diagnostic=diagnostic,log_prefix=prefix))
    trace=a.output/(prefix+'-fuse-daemon.stderr')
    activated=trace.exists() and 'activating writeback' in trace.read_text()
    if diagnostic:
        complete=complete and (activated==(label=='candidate' or a.baseline_writeback))
    else:
        complete=complete and trace.exists() and 'unique:' not in trace.read_text()
    record=dict(prefix=prefix,label=label,profile=profile,cpus=cpus,block=block,diagnostic=diagnostic,
                command=command,pid=child.pid,returncode=child.returncode,stop_reason=reason,
                wall_seconds=round(time.monotonic()-began,3),complete=complete,writeback_activation_logged=activated)
    record.update(host_load_before=load_before,host_load_after=os.getloadavg())
    report['runs'].append(record)
    save()
    print(json.dumps({k:v for k,v in record.items() if k!='command'}),flush=True)
    shutil.rmtree(store,ignore_errors=True)
    return complete

try:
    # Diagnostics prove capability negotiation, outside all timed samples.
    for label in ('baseline','candidate'):
        assert run(label,'writeback',2,'capability',True), 'capability preflight failed; no timed samples admitted'
    for profile in a.profiles:
        for cpus in a.cpus:
            for block in range(a.blocks):
                for label in (('baseline','candidate') if block%2==0 else ('candidate','baseline')):
                    run(label,profile,cpus,block)
    assert all(r['complete'] for r in report['runs']), 'incomplete or failed measurement scope'
    for diagnostic in (True, False):
        checksums={r['checksum'] for r in report['rows'] if r['kind']=='ordinary' and r['diagnostic']==diagnostic}
        assert len(checksums)==1,(diagnostic,checksums)
    transfers=[r for r in report['rows'] if r['kind']=='transfer' and not r['diagnostic'] and r['sample']>0]
    for r in transfers: r['write_sync_ns']=r['write_ns']+r['sync_ns']
    for profile in a.profiles:
        for cpus in a.cpus:
            for metric in ('write_sync_ns','write_ns','sync_ns','warm_read_ns','reopen_read_ns'):
                values={label:[r[metric] for r in transfers if r['label']==label and r['profile']==profile and r['cpus']==cpus]
                        for label in ('baseline','candidate')}
                medians={label:statistics.median(v) for label,v in values.items()}
                row=dict(kind='transfer',profile=profile,cpus=cpus,metric=metric,raw_ns=values,median_ns=medians,
                         candidate_over_baseline=medians['candidate']/medians['baseline'])
                if metric!='sync_ns': row['MiB_per_second']={k:8e9/v for k,v in medians.items()}
                report['summary'].append(row)
    ordinary=[r for r in report['rows'] if r['kind']=='ordinary' and not r['diagnostic'] and r['sample']>0]
    for cpus in a.cpus:
        for metric in ('cpu_ns','metadata_ns','file_ns','pipe_ns'):
            groups=collections.defaultdict(list)
            for row in ordinary:
                if row['cpus']==cpus: groups[(row['label'],row['profile'],row['block'],row['sample'])].append(row[metric])
            values={label:[max(v) for k,v in groups.items() if k[0]==label] for label in ('baseline','candidate')}
            medians={label:statistics.median(v) for label,v in values.items()}
            report['summary'].append(dict(kind='ordinary',cpus=cpus,metric=metric,raw_max_worker_ns=values,median_ns=medians,
                                          candidate_over_baseline=medians['candidate']/medians['baseline']))
    report['passed']=True
    print(json.dumps(report['summary'],indent=2))
finally:
    save()
