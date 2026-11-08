#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Real static ELF: private/shared file memory and Vulkan host mapping, with peers."""
import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--backend',choices=('hvf','interpreter'),required=True)
p.add_argument('--runner',type=Path,required=True)
p.add_argument('--fixture',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
p.add_argument('--cpus',nargs='+',type=int,default=(1,4,12))
p.add_argument('--modes',nargs='+',choices=('read','write','gpu'),default=('read','write','gpu'))
p.add_argument('--rounds',type=int,default=20)
p.add_argument('--gpu-rounds',type=int,default=4)
p.add_argument('--baseline',action='store_true',help='Record joined baseline counts without source-only assertions.')
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
empty=a.output/'rootfs';(empty/'tmp').mkdir(parents=True,exist_ok=True)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
report=dict(backend=a.backend,runner_sha256=sha(a.runner),fixture_sha256=sha(a.fixture),passed=False,cases=[])
try:
    for cpus in a.cpus:
        for mode in a.modes:
            rounds=a.gpu_rounds if mode=='gpu' else a.rounds
            command=[str(a.runner.resolve()),'--cpus',str(cpus),'--network','off','--metrics','--exec-file',f'/fixture={a.fixture.resolve()}']
            if mode=='gpu':command+=['--vulkan']
            command+=(['--rootfs',str(empty.resolve()),'--timeout','60','--no-pty','--no-windows'] if a.backend=='hvf'
                      else ['--memory','--guest','--accelerator','pure','--max-instructions','1000000000'])
            result=subprocess.run(command+['/fixture',mode,str(rounds)],capture_output=True,timeout=70,
                                  env=dict(os.environ,TRAMES_MAC_BUNDLED='1'))
            label=f'{mode}-{cpus}';(a.output/(label+'.stdout')).write_bytes(result.stdout);(a.output/(label+'.stderr')).write_bytes(result.stderr)
            row=dict(cpus=cpus,mode=mode,code=result.returncode);report['cases'].append(row)
            assert result.returncode==0,(label,result.stderr[-4000:])
            measured=json.loads(result.stdout.decode().splitlines()[-1]);row['measurement']=measured
            assert measured['mode']==mode and measured['rounds']==rounds and measured['peer_calls']>0
            prefix='trames mac execution paths: ' if a.backend=='hvf' else 'trames metrics: '
            metrics=json.loads(next(line[len(prefix):] for line in result.stderr.decode().splitlines() if line.startswith(prefix)))
            row['metrics']=metrics;assert metrics['service_requests']==metrics['serviced_syscalls']==0
            if a.backend=='hvf':
                prefix='trames mac cohort-return native: '
                returned=ast.literal_eval(next(line[len(prefix):] for line in result.stderr.decode().splitlines() if line.startswith(prefix)))
                row['cohort_return_native']={str(k):v for k,v in returned.items()}
                if not a.baseline:assert not returned,row
                if mode=='gpu':
                    maps=int(re.search(r'trames mac shared GPU: maps=(\d+)',result.stderr.decode()).group(1))
                    row['shared_gpu_maps']=maps;assert maps==rounds
            else:
                prefix='trames native metrics: '
                native=json.loads(next(line[len(prefix):] for line in result.stderr.decode().splitlines() if line.startswith(prefix)))
                row['native']=native
                assert measured['accelerated']==1
                assert native['faults']==native['rejected']==0
                if cpus>1:
                    assert native['worker_calls']>0
                    if mode=='read' and not a.baseline:
                        assert native['checkpoint_retries']>=rounds
                        assert metrics['parallel_rounds']<=2,metrics
            print(label,'PASS',flush=True)
    report['passed']=True
finally:
    (a.output/'results.json').write_text(json.dumps(report,indent=2)+'\n')
