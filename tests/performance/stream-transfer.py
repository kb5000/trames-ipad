#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checked transfer timings and separate diagnostics; no compilation during samples."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import statistics
import subprocess
import tempfile
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--before',type=Path,required=True)
p.add_argument('--after',type=Path,required=True)
p.add_argument('--fixture',type=Path,required=True)
p.add_argument('--native',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--samples',type=int,default=5)
p.add_argument('--iterations',type=int,default=500)
p.add_argument('--workers',nargs='+',type=int,default=[1,2,4])
p.add_argument('--storage',nargs='+',choices=['memory','private'],default=['memory','private'])
p.add_argument('--modes',nargs='+',default=['pipe-pipe','file-pipe','file-unix','socket-pipe'])
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
empty=a.output/'empty-rootfs';empty.mkdir(exist_ok=True)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
report=dict(fixture_sha256=sha(a.fixture),native_sha256=sha(a.native),runners={key:sha(path) for key,path in [('before',a.before),('after',a.after)]},rows=[],summary=[])
def save():
    (a.output/'results.json').write_text(json.dumps(report,indent=2)+'\n')
def run(label,storage,mode,workers,sample,diagnostics=False):
    name=f'{storage}-{mode}-{workers}-{sample}-{label}'+('-metrics' if diagnostics else '')
    with tempfile.TemporaryDirectory(prefix='transfer-store-',dir=a.output) as temp:
        runner=a.before if label=='before' else a.after
        command=[str(runner.resolve()),'--rootfs',str(empty.resolve()),'--cpus',str(workers),'--network','off','--timeout','40','--no-pty','--no-windows']
        if storage=='private':command+=['--storage',str(Path(temp).resolve()/'store')]
        if diagnostics:command+=['--metrics']
        command+=['--exec-file',f'/transfer={a.fixture.resolve()}','/transfer','--bench',mode,str(workers),str(a.iterations)]
        began=time.monotonic();r=subprocess.run(command,capture_output=True,timeout=50);wall=time.monotonic()-began
    (a.output/(name+'.stdout')).write_bytes(r.stdout);(a.output/(name+'.stderr')).write_bytes(r.stderr)
    assert r.returncode==0,(name,r.returncode,r.stdout,r.stderr[-3000:])
    result=json.loads(next(line for line in r.stdout.splitlines() if line.startswith(b'{')))
    row=dict(result,label=label,storage=storage,mode=mode,workers=workers,sample=sample,diagnostics=diagnostics,wall_seconds=wall)
    if diagnostics:
        prefix='trames mac execution paths: '
        row['paths']=json.loads(next(line[len(prefix):] for line in r.stderr.decode().splitlines() if line.startswith(prefix)))
        row['coordination']=[line for line in r.stderr.decode().splitlines() if line.startswith('trames mac cohorts:')]
    report['rows'].append(row);save();print(json.dumps(row),flush=True)
for storage in a.storage:
    for mode in a.modes:
        for workers in a.workers:
            for sample in range(a.samples):
                for label in (['before','after'] if sample%2==0 else ['after','before']):run(label,storage,mode,workers,sample)
            for label in ['before','after']:run(label,storage,mode,workers,'diagnostic',True)
for mode in a.modes:
    for workers in a.workers:
        for sample in range(a.samples):
            r=subprocess.run([str(a.native.resolve()),'--bench',mode,str(workers),str(a.iterations)],capture_output=True,timeout=30,check=True)
            row=dict(json.loads(r.stdout),label='native-macos',storage='native-filesystem',mode=mode,workers=workers,sample=sample,diagnostics=False)
            report['rows'].append(row)
for storage in a.storage:
    for mode in a.modes:
        for workers in a.workers:
            medians={label:statistics.median(r['longest_worker_ms'] for r in report['rows'] if r['label']==label and r['mode']==mode and r['workers']==workers and not r['diagnostics'] and (r['storage']==storage or label=='native-macos')) for label in ['before','after','native-macos']}
            report['summary'].append(dict(storage=storage,mode=mode,workers=workers,median_ms=medians,speedup=medians['before']/medians['after']))
save();print(json.dumps(report['summary'],indent=2))
