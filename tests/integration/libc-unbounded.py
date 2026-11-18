#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Complete-request musl/Bionic-family semantics and paired product benchmarks.

The portable fixture is tests/fixtures/android-bionic.c, compiled for musl.
No Android boot, registered environment or device is touched.
"""
import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[2]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--probe',type=Path,required=True)
    p.add_argument('--libc',type=Path,default=ROOT/'target/musl-hybrid/libc.so')
    p.add_argument('--runner',type=Path,default=ROOT/'target/release/trames-run')
    p.add_argument('--rootfs',type=Path,default=ROOT/'target/alpine-download/alpine-minirootfs-3.22.6-aarch64.tar.gz')
    p.add_argument('--output',type=Path,default=ROOT/'target/native-unbounded/musl')
    p.add_argument('--oracle',type=Path,help='stdout from the original-path run under QEMU')
    p.add_argument('--baseline-runner',type=Path)
    p.add_argument('--baseline-libc',type=Path)
    p.add_argument('--bench-rounds',type=int,default=3)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    if a.bench_rounds<0 or bool(a.baseline_runner)!=bool(a.baseline_libc):
        p.error('bench rounds must be nonnegative; supply both baseline runner and libc')
    result={'cases':{},'benchmark':{},'inputs':{str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in [a.probe,a.libc,a.runner,Path(__file__),ROOT/'tests/fixtures/android-bionic.c']}}
    def run(name,enabled,mode=None,baseline=False,legacy_host=False):
        command=[str((a.baseline_runner if baseline or legacy_host else a.runner).resolve()),'--memory','--rootfs',str(a.rootfs.resolve()),'--musl-libc',str((a.baseline_libc if baseline else a.libc).resolve()),'--accelerator','pure' if enabled else 'off','--cpus','4','--metrics','--max-instructions','4000000000','--exec-file','/libc-probe='+str(a.probe.resolve()),'--guest','/libc-probe']
        if mode: command.append(mode)
        done=subprocess.run(command,capture_output=True,text=True,timeout=120)
        (a.output/(name+'.log')).write_text(done.stdout+done.stderr)
        if done.returncode:raise RuntimeError(name+': '+str(done.returncode)+' '+done.stderr[-1800:])
        metrics=next(json.loads(l.split(': ',1)[1])for l in done.stderr.splitlines()if l.startswith('trames native metrics: '))
        result['cases'][name]={'exit':done.returncode,'native':metrics}
        return done.stdout,metrics
    reference,_=run('original',False)
    if a.oracle:assert reference==a.oracle.read_text(),'QEMU/original interpreter mismatch'
    actual,metrics=run('native',True)
    if actual!=reference:
        differences=[dict(original=x,native=y) for x,y in zip(reference.splitlines(),actual.splitlines())if x!=y]
        (a.output/'differences.json').write_text(json.dumps(differences,indent=2))
        raise RuntimeError('Original/native mismatch: '+str(differences[:4]))
    assert 'PASS musl 44 functions' in actual
    assert min(metrics['function_calls'][19:])>0 and not any(metrics['function_fallbacks'][19:])
    assert metrics['calls']==metrics['worker_calls'] and metrics['faults']==metrics['rejected']==0
    variants=[('original',False,False),('native',True,False)]
    if a.baseline_runner:
        assert a.baseline_libc
        variants.append(('previous',True,True))
        compatible,old_metrics=run('new-libc-previous-host',True,legacy_host=True)
        assert compatible==reference and old_metrics['faults']==old_metrics['rejected']==0
        for f in [a.baseline_runner,a.baseline_libc]:result['inputs'][str(f)]=hashlib.sha256(f.read_bytes()).hexdigest()
    for round in range(a.bench_rounds):
        for name,enabled,baseline in variants[::1 if round%2==0 else -1]:
            stdout,_=run(f'bench-{name}-{round+1}',enabled,'bench',baseline)
            for line in stdout.splitlines():
                if line.startswith('BENCH '):
                    _,operation,size,count,ns=line.split()
                    entry=result['benchmark'].setdefault(operation,{}).setdefault(name,dict(bytes=int(size),count=int(count),samples_ns=[]))
                    entry['samples_ns'].append(int(ns))
    for variants in result['benchmark'].values():
        for entry in variants.values():entry['nanoseconds']=statistics.median(entry['samples_ns'])
        variants['speedup_original']=variants['original']['nanoseconds']/variants['native']['nanoseconds']
        if 'previous'in variants:variants['speedup_previous']=variants['previous']['nanoseconds']/variants['native']['nanoseconds']
    (a.output/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS musl exact original/native results; complete function calls on workers; '+str(len(result['benchmark']))+' benchmarks')

if __name__=='__main__':main()
