#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Counterbalanced shared signal control and delivery timings; diagnostics run separately."""
import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import time

p=argparse.ArgumentParser(description=__doc__)
for name in ['before','after','fixture','native','output']:
    p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--samples',type=int,default=5)
p.add_argument('--iterations',type=int,default=500)
p.add_argument('--workers',type=int,nargs='+',default=[1,2,4])
p.add_argument('--kinds',type=int,nargs='+',default=[0,1,2,3,4,5])
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
empty=a.output/'empty-rootfs';empty.mkdir(exist_ok=True)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
report=dict(fixture_sha256=sha(a.fixture),native_macos_sha256=sha(a.native),runners={label:sha(path) for label,path in [('before',a.before),('after',a.after)]},rows=[],summary=[])
def save():
    (a.output/'results.json').write_text(json.dumps(report,indent=2)+'\n')
def run(label,kind,workers,sample,diagnostics=False):
    name=f'{kind}-{workers}-{sample}-{label}'+('-metrics' if diagnostics else '')
    binary=a.before if label=='before' else a.after
    command=[str(binary.resolve()),'--rootfs',str(empty.resolve()),'--cpus',str(workers),'--network','off','--timeout','40','--no-pty','--no-windows']
    if diagnostics:command+=['--metrics']
    command+=['--exec-file',f'/signals={a.fixture.resolve()}','/signals','--bench',str(kind),str(workers),str(a.iterations)]
    began=time.monotonic();r=subprocess.run(command,capture_output=True,timeout=50);wall=time.monotonic()-began
    (a.output/(name+'.stdout')).write_bytes(r.stdout);(a.output/(name+'.stderr')).write_bytes(r.stderr)
    assert r.returncode==0,(name,r.stdout,r.stderr[-3000:])
    value=json.loads(next(line for line in r.stdout.splitlines() if line.startswith(b'{')))
    row=dict(value,label=label,sample=sample,diagnostics=diagnostics,wall_seconds=wall)
    if diagnostics:
        row['diagnostic_stderr']=r.stderr.decode()
        prefix='trames mac execution paths: '
        row['paths']=json.loads(next(line[len(prefix):] for line in r.stderr.decode().splitlines() if line.startswith(prefix)))
        row['cohorts']=[line for line in r.stderr.decode().splitlines() if line.startswith('trames mac cohorts:')]
    report['rows'].append(row);save();print(json.dumps(row),flush=True)
for kind in a.kinds:
    for workers in a.workers:
        for sample in range(a.samples):
            for label in (['before','after'] if sample%2==0 else ['after','before']):run(label,kind,workers,sample)
        for label in ['before','after']:run(label,kind,workers,'diagnostic',True)
# Darwin provides the same POSIX mask/action/pthread_kill/sigwait interfaces.
for kind in [k for k in a.kinds if k<5]:
    for workers in a.workers:
        # The native macOS self-send/sigwait multiworker case does not
        # complete on this host. Linux uses its exact ELF reference instead.
        if kind==4 and workers>1: continue
        for sample in range(a.samples):
            r=subprocess.run([str(a.native.resolve()),'--bench',str(kind),str(workers),str(a.iterations)],capture_output=True,check=True,timeout=30)
            report['rows'].append(dict(json.loads(r.stdout),label='native-macos',sample=sample,diagnostics=False))
for kind in a.kinds:
    for workers in a.workers:
        values={label:[r['longest_worker_ms'] for r in report['rows'] if r['label']==label and r['kind']==kind and r['workers']==workers and not r['diagnostics']] for label in ['before','after','native-macos']}
        medians={label:statistics.median(v) if v else None for label,v in values.items()}
        report['summary'].append(dict(kind=kind,workers=workers,median_ms=medians,speedup=medians['before']/medians['after']))
save();print(json.dumps(report['summary'],indent=2))
