#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Same original libfuse daemon and ordinary clients on Linux, interpreter or HVF."""
import argparse, collections, hashlib, json, os
from pathlib import Path
import shutil, signal, subprocess, tempfile, time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--backend',choices=['native','interpreter','hvf'],required=True)
for name in ('rootfs','tools','client','workload','wrapper','output'):
    p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--runner',type=Path)
p.add_argument('--cpus',nargs='+',type=int,default=[1,2,4])
p.add_argument('--profiles',nargs='+',choices=['lowlevel','writeback','highlevel'],default=['lowlevel','writeback','highlevel'])
p.add_argument('--inside-root',type=Path,help=argparse.SUPPRESS)
p.add_argument('--inside-profile',help=argparse.SUPPRESS)
a=p.parse_args()
if a.inside_root:
    assert a.backend=='native' and a.inside_root.name.startswith('trames-fuse-closeout-')
    subprocess.run(['mount','--make-rprivate','/'],check=True)
    for name in ('fuse','null','zero','urandom','random'):
        dest=a.inside_root/'dev'/name;dest.touch(exist_ok=True)
        subprocess.run(['mount','--bind','/dev/'+name,str(dest)],check=True)
    subprocess.run(['mount','-t','proc','proc',str(a.inside_root/'proc')],check=True)
    raise SystemExit(subprocess.run(['chroot',str(a.inside_root),'/bin/sh','/opt/fuse-tool/acceptance.sh',a.inside_profile],timeout=140).returncode)
assert a.backend=='native' or a.runner is not None
assert all(n>0 for n in a.cpus)
a.output.mkdir(parents=True,exist_ok=False)
sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
inputs={n:str(getattr(a,n).resolve()) for n in ('rootfs','client','workload','wrapper')}
for n in ('passthrough','passthrough_ll'):inputs[n]=str((a.tools/n).resolve())
if a.runner:inputs['runner']=str(a.runner.resolve())
report=dict(passed=False,backend=a.backend,inputs={n:dict(path=q,sha256=sha(Path(q))) for n,q in inputs.items()},runs=[])
cases={'busybox','mapping','copyout','metadata','special','aio','copy','transfer','cached','ring','unmount','daemon-stop','daemon-exit'}
def save(): (a.output/'results.json').write_text(json.dumps(report,indent=2)+'\n')
began_all=time.monotonic()
try:
    for cpus in ([0] if a.backend=='native' else a.cpus):
        for profile in a.profiles:
            assert time.monotonic()-began_all<900,'overall acceptance budget'
            assert shutil.disk_usage(a.output).free>3*1024**3,'disk floor'
            label=f'{profile}-{cpus}';store=a.output/(label+'-store');native=None
            if a.backend=='native':
                native=tempfile.TemporaryDirectory(prefix='trames-fuse-closeout-');root=Path(native.name)
                subprocess.run(['tar','-xzf',inputs['rootfs'],'--no-same-owner','-C',str(root)],check=True)
                tools=root/'opt/fuse-tool';tools.mkdir(parents=True)
                for n,source in [('passthrough','passthrough'),('passthrough_ll','passthrough_ll'),('client','client'),('workload.sh','workload'),('acceptance.sh','wrapper')]:
                    shutil.copy2(inputs[source],tools/n);(tools/n).chmod(0o755)
                command=['sudo','-n','unshare','--mount','--propagation','private','--net','--pid','--fork','--kill-child=KILL','--','python3',str(Path(__file__).resolve()),*os.sys.argv[1:],'--inside-root',str(root),'--inside-profile',profile]
            else:
                command=[inputs['runner'],'--storage',str(store.resolve()),'--rootfs',inputs['rootfs'],'--network','off','--cpus',str(cpus)]
                if a.backend=='hvf':command+=['--no-pty','--no-windows','--metrics','--memory-mib','2048','--timeout','140']
                else:command+=['--virtual-network','2','--memory-budget','2GiB','--process-memory-budget','1GiB','--max-instructions','20000000000']
                for n,source in [('passthrough','passthrough'),('passthrough_ll','passthrough_ll'),('client','client'),('workload.sh','workload'),('acceptance.sh','wrapper')]:command+=['--exec-file',f'/opt/fuse-tool/{n}={inputs[source]}']
                if a.backend=='interpreter':command+=['--guest','--']
                command+=['/bin/sh','/opt/fuse-tool/acceptance.sh',profile]
            started=time.monotonic();reason=None
            with (a.output/(label+'.stdout')).open('w') as stdout,(a.output/(label+'.stderr')).open('w') as stderr:
                child=subprocess.Popen(command,stdout=stdout,stderr=stderr,start_new_session=True,env=dict(os.environ,TRAMES_MAC_BUNDLED='1'))
                try:child.wait(timeout=150)
                except subprocess.TimeoutExpired:
                    reason='timeout';os.killpg(child.pid,signal.SIGTERM)
                    try:child.wait(timeout=5)
                    except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
            text=(a.output/(label+'.stdout')).read_text(errors='replace');passed=[l.split()[1] for l in text.splitlines() if l.startswith('CASE ') and l.endswith(' PASS')]
            complete=child.returncode==0 and reason is None and collections.Counter(passed)==collections.Counter(cases) and 'FUSE_ACCEPTANCE_RESULT:0' in text and f'WORKLOAD {profile} FAILED=0' in text
            if profile=='writeback':complete=complete and 'activating writeback' in text
            record=dict(profile=profile,cpus=cpus,command=command,pid=child.pid,returncode=child.returncode,stop_reason=reason,seconds=round(time.monotonic()-started,3),complete=complete,cases=passed)
            if a.backend=='hvf':
                err=(a.output/(label+'.stderr')).read_text();metrics=[json.loads(l.split(': ',1)[1]) for l in err.splitlines() if l.startswith('trames mac execution paths: ')]
                record['execution_paths']=metrics;complete=record['complete']=complete and len(metrics)==1 and metrics[0].get('syscalls',0)>0
            report['runs'].append(record);save();print(json.dumps(record),flush=True)
            if native:native.cleanup()
            else:shutil.rmtree(store,ignore_errors=True)
    report['passed']=all(r['complete'] for r in report['runs'])
finally:save()
raise SystemExit(0 if report['passed'] else 1)
