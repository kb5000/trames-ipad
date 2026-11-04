#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Counterbalanced event-wait timings; diagnostics run separately."""
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
p.add_argument('--kinds',type=int,nargs='+',default=[0,1,2,3])
p.add_argument('--masked',type=int,nargs='+',default=[0,1])
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
empty=a.output/'empty-rootfs';empty.mkdir(exist_ok=True)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
report=dict(fixture_sha256=sha(a.fixture),native_macos_sha256=sha(a.native),runners={label:sha(path) for label,path in [('before',a.before),('after',a.after)]},rows=[],summary=[])
def save():
    (a.output/'results.json').write_text(json.dumps(report,indent=2)+'\n')
def run(label,kind,masked,workers,sample,diagnostics=False):
    name=f'{kind}-{masked}-{workers}-{sample}-{label}'+('-metrics' if diagnostics else '')
    binary=a.before if label=='before' else a.after
    command=[str(binary.resolve()),'--rootfs',str(empty.resolve()),'--cpus',str(workers),'--network','off','--timeout','40','--no-pty','--no-windows']
    if diagnostics:command+=['--metrics']
    command+=['--exec-file',f'/waits={a.fixture.resolve()}','/waits','--bench',str(kind),str(masked),str(workers),str(a.iterations)]
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
    for masked in a.masked:
        for workers in a.workers:
            for sample in range(a.samples):
                for label in (['before','after'] if sample%2==0 else ['after','before']):run(label,kind,masked,workers,sample)
            for label in ['before','after']:run(label,kind,masked,workers,'diagnostic',True)
# Darwin has native pselect; it has no Linux epoll/ppoll API. Do not substitute
# another API and present that as a native score for the unsupported families.
if 0 in a.kinds:
    for masked in a.masked:
        for workers in a.workers:
            for sample in range(a.samples):
                r=subprocess.run([str(a.native.resolve()),'--bench','0',str(masked),str(workers),str(a.iterations)],capture_output=True,check=True,timeout=30)
                report['rows'].append(dict(json.loads(r.stdout),label='native-macos',sample=sample,diagnostics=False))
for kind in a.kinds:
    for masked in a.masked:
        for workers in a.workers:
            values={label:[r['longest_worker_ms'] for r in report['rows'] if r['label']==label and r['kind']==kind and r['masked']==masked and r['workers']==workers and not r['diagnostics']] for label in ['before','after','native-macos']}
            medians={label:statistics.median(v) if v else None for label,v in values.items()}
            report['summary'].append(dict(kind=kind,masked=masked,workers=workers,median_ms=medians,speedup=medians['before']/medians['after']))
save();print(json.dumps(report['summary'],indent=2))
