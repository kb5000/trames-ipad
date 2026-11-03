#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Counterbalanced, whole io_uring workflows on two signed Mac runners."""
import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--before',type=Path,required=True)
p.add_argument('--after',type=Path,required=True)
p.add_argument('--fixture',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--samples',type=int,default=3)
p.add_argument('--iterations',type=int,default=1000)
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
(a.output/'empty-rootfs').mkdir()
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
report=dict(passed=False,fixture_sha256=sha(a.fixture),runners={k:sha(v) for k,v in [('before',a.before),('after',a.after)]},rows=[])
def run(label,runner,mode,workers,sample,diagnostics):
    name=f'{mode}-{workers}-{sample}-{label}'+('-metrics' if diagnostics else '')
    cmd=[str(runner.resolve()),'--rootfs',str((a.output/'empty-rootfs').resolve()),'--cpus',str(workers),'--network','off','--timeout','20','--no-pty','--no-windows','--exec-file',f'/uring-bench={a.fixture.resolve()}']
    if diagnostics:cmd+=['--metrics']
    cmd+=['/uring-bench',mode,str(workers),str(a.iterations)]
    start=time.monotonic();r=subprocess.run(cmd,capture_output=True,timeout=25);wall=time.monotonic()-start
    (a.output/(name+'.stderr')).write_bytes(r.stderr)
    (a.output/(name+'.stdout')).write_bytes(r.stdout)
    assert r.returncode==0,(name,r.returncode,r.stdout,r.stderr[-3000:])
    value=json.loads(r.stdout)
    row=dict(case=mode,sample=sample,runner=label,diagnostics=diagnostics,wall_seconds=wall,**value)
    if diagnostics:
        prefix='trames mac execution paths: '
        row['metrics']=json.loads(next(line[len(prefix):] for line in r.stderr.decode().splitlines() if line.startswith(prefix)))
    report['rows'].append(row)
    print(name,round(value['longest_worker_ns']/1e6,3),'ms',flush=True)
try:
    for mode in ['nop','register','statx']:
        for workers in [1,2,4]:
            for sample in range(a.samples):
                order=[('before',a.before),('after',a.after)]
                if sample%2:order.reverse()
                for label,runner in order:run(label,runner,mode,workers,sample,False)
            for label,runner in [('before',a.before),('after',a.after)]:run(label,runner,mode,workers,0,True)
    medians=[]
    for mode in ['nop','register','statx']:
        for workers in [1,2,4]:
            row=dict(case=mode,workers=workers)
            for label in ['before','after']:
                samples=[r for r in report['rows'] if r['case']==mode and r['workers']==workers and r['runner']==label and not r['diagnostics']]
                row[label+'_worker_ms']=statistics.median(r['longest_worker_ns']/1e6 for r in samples)
                row[label+'_wall_ms']=statistics.median(r['wall_seconds']*1000 for r in samples)
            row['speedup']=row['before_worker_ms']/row['after_worker_ms'];medians.append(row)
    report['medians']=medians;report['passed']=True
finally:
    (a.output/'results.json').write_text(json.dumps(report,indent=2)+'\n')
