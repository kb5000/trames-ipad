#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""One paired batch: allocator hot/cold transitions and unchanged Vim/Python."""
import argparse
from contextlib import contextmanager
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('nonjit',ROOT/'tests/performance/nonjit.py')
nonjit=importlib.util.module_from_spec(spec);spec.loader.exec_module(nonjit)
digest=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def metrics(stderr):
    lines=stderr.decode().splitlines();assert len(lines)==2,stderr[-4000:]
    native,runtime=[json.loads(line.split(': ',1)[1]) for line in lines]
    assert not native['faults'] and not native['rejected'],native
    return dict(native=native,runtime=runtime)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline',type=Path,required=True);p.add_argument('--baseline-commit',required=True)
    p.add_argument('--baseline-native-allocator',action='store_true',help='Enable allocator routing on the baseline for an adapter-only comparison')
    p.add_argument('--include-original-allocator',action='store_true',help='Also compare the current runtime with allocator routing disabled')
    p.add_argument('--vim-store',type=Path,required=True);p.add_argument('--python-store',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    current=Path(os.getenv('TRAMES_RUNNER',ROOT/'target/release/trames-run'));libc=ROOT/'target/musl-hybrid/libc.so';fixture=ROOT/'tests/fixtures/bin/musl-allocator'
    if a.output.exists():p.error('output already exists; preserve prior evidence and select a new path')
    for path in (a.baseline,current,libc,fixture,a.vim_store/'manifest',a.python_store/'manifest'):
        if not path.is_file():p.error('missing fixed input: '+str(path))
    a.output.parent.mkdir(parents=True,exist_ok=True)
    rows=[]
    samples=[]
    quota=Path('/sys/fs/cgroup/cpu.max')
    host=dict(platform=platform.platform(),arch=platform.machine(),
              cpu_affinity=len(os.sched_getaffinity(0)) if hasattr(os,'sched_getaffinity') else os.cpu_count(),
              cpu_quota=quota.read_text().strip() if quota.is_file() else None)
    identities=dict(baseline_sha256=digest(a.baseline),current_sha256=digest(current),libc_sha256=digest(libc),
                    fixture_sha256=digest(fixture),vim_manifest_sha256=digest(a.vim_store/'manifest'),
                    python_manifest_sha256=digest(a.python_store/'manifest'))
    def checkpoint(state, **extra):
        report=dict(state=state,baseline_commit=a.baseline_commit,host=host,inputs=identities,
                    completed_rows=rows,samples=samples,**extra)
        temporary=a.output.with_suffix(a.output.suffix+'.new')
        temporary.write_text(json.dumps(report,indent=2)+'\n');temporary.replace(a.output)
    checkpoint('running')
    @contextmanager
    def preserve_failure():
        try:yield
        except BaseException as error:
            checkpoint('failed',failure=dict(type=type(error).__name__,message=str(error)[:1000]))
            raise
    with preserve_failure(), tempfile.TemporaryDirectory(prefix='trames-allocator-bench-') as directory:
        for name,cpus in [('allocator-short',1),('allocator',1),('allocator',4),('vim-default',1),('python-imports',1)]:
            modes={}
            for sample in range(3):
                choices=[('baseline',a.baseline),('current',current)]
                if a.include_original_allocator:
                    choices.append(('original',current))
                    choices=choices[sample:]+choices[:sample]
                elif sample%2:choices.reverse()
                for mode,binary in choices:
                    command=[str(binary),'--network','off','--cpus',str(cpus),'--musl-libc',str(libc),'--accelerator','pure','--metrics','--max-instructions','100000000000','--env','HOME=/root','--env','TERM=xterm-256color']
                    if mode=='current' or (mode=='baseline' and a.baseline_native_allocator):command+=['--native-allocator']
                    if name.startswith('allocator'):
                        command+=['--memory','--rootfs',str(ROOT/'target/alpine-download/alpine-minirootfs-3.22.6-aarch64.tar.gz'),'--exec-file','/bin/allocator='+str(fixture),'--guest','/bin/allocator',*(['short','200000'] if name=='allocator-short' else ['bench','96'])]
                    else:
                        store=Path(directory)/'store';shutil.copytree(a.vim_store if name=='vim-default' else a.python_store,store)
                        command+=['--storage',str(store)]
                    if name=='vim-default':
                        command+=['--pty','--guest','/usr/bin/vim','/tmp/performance-empty.txt'];seconds,profile=nonjit.vim(command,metrics)
                    else:
                        if name=='python-imports':command+=['--guest','/usr/bin/python3','-c',nonjit.PYTHON]
                        try:result=subprocess.run(command,capture_output=True,timeout=300)
                        except subprocess.TimeoutExpired:
                            checkpoint('failed',failure=dict(workload=name,mode=mode,sample=sample,reason='300 second deadline'))
                            raise
                        prefix=a.output.parent/(a.output.stem+f'-{name}-{cpus}-{sample}-{mode}')
                        Path(str(prefix)+'.stdout').write_bytes(result.stdout)
                        Path(str(prefix)+'.stderr').write_bytes(result.stderr)
                        expected=b'musl-allocator-ok\n' if name.startswith('allocator') else b'python: 42\n'
                        assert (result.returncode,result.stdout)==(0,expected),(name,mode,result.returncode,result.stdout[-1000:],result.stderr[-4000:])
                        profile=metrics(result.stderr);seconds=profile['runtime']['seconds']
                    if mode=='original' or (mode=='baseline' and not a.baseline_native_allocator):assert not profile['native'].get('allocator_calls',0),profile
                    else:assert profile['native']['allocator_allocations']>0,profile
                    modes.setdefault(mode,[]).append(dict(seconds=seconds,**profile))
                    samples.append(dict(workload=name,cpus=cpus,sample=sample,mode=mode,seconds=seconds,**profile))
                    checkpoint('running')
                    print(name,cpus,sample,mode,round(seconds,6),flush=True)
                    if not name.startswith('allocator'):shutil.rmtree(store)
            medians={mode:statistics.median(x['seconds'] for x in samples) for mode,samples in modes.items()}
            rows.append(dict(workload=name,cpus=cpus,samples=modes,median_seconds=medians,speedup=medians['baseline']/medians['current']))
            if a.include_original_allocator:
                rows[-1]['native_vs_original_ratio']=medians['original']/medians['current']
    checkpoint('complete',baseline_native_allocator=a.baseline_native_allocator,include_original_allocator=a.include_original_allocator,
               rows=rows,limits='Three alternating samples (rotating order with the optional third mode), identical guest ELF/libc/stores. original, when present, is the current runtime with allocator routing disabled. Host platform/architecture are recorded; no physical iPad claim. Short calls use one thread and 200000 allocations/frees of 64 bytes. Contention uses four threads, 96 rounds of 192 allocations/thread, cross-thread frees and sampled payload checks. Vim measures spawn to first cursor visibility; others use runtime guest seconds, excluding rootfs import.')
    print([(row['workload'],row['cpus'],round(row['speedup'],3)) for row in rows],flush=True)
if __name__=='__main__':main()
