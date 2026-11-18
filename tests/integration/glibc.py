#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Pinned Debian glibc: original/native differential, ABI and reversible install.

Uses tiny disposable VFS stores and the original Debian loader. No registered
Linux environment, Android boot, Apple device or host account is modified.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import statistics
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runner',type=Path,default=ROOT/'target/release/trames-run')
    p.add_argument('--inputs',type=Path,default=ROOT/'target/glibc-hybrid')
    p.add_argument('--probe',type=Path)
    p.add_argument('--locale',type=Path)
    p.add_argument('--interpose',type=Path)
    p.add_argument('--oracle',type=Path)
    p.add_argument('--output',type=Path,default=ROOT/'target/glibc-hybrid/acceptance')
    p.add_argument('--bench-rounds',type=int,default=3)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    if a.bench_rounds<0:p.error('bench rounds must be nonnegative')
    sdk=a.inputs/'sysroot'if(a.inputs/'sysroot').exists()else a.inputs/'binary'
    probe=a.probe or a.inputs/'fixtures/glibc-probe'
    locales=a.locale or sdk/'usr/lib/locale'
    interpose=a.interpose or a.inputs/'fixtures/interpose.so'
    spec=importlib.util.spec_from_file_location('glibc_builder',ROOT/'scripts/build-glibc-hybrid.py');builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
    original=sdk/'usr/lib/aarch64-linux-gnu/libc.so.6';bundle=a.output/'bundle';manifest=builder.build(original,bundle,'clang')
    elf=builder.elflib.Elf(original.read_bytes());native=builder.elflib.Elf((bundle/'libc-nativecall.so').read_bytes())
    for name,section in elf.sections.items():
        if name.startswith(('.dynsym','.dynstr','.gnu.version','.rela.','.dynamic','.tdata')):
            assert elf.section_bytes(section)==native.section_bytes(native.sections[name]),name
    result={'version':builder.VERSION,'manifest':manifest,'cases':{},'benchmark':{},'abi':'Original dynamic symbol/version/relocation/TLS bytes retained; original loader unchanged.',
            'inputs':{str(f):hashlib.sha256(f.read_bytes()).hexdigest()for f in [original,probe,a.runner,Path(__file__),ROOT/'tests/fixtures/glibc.c',ROOT/'tests/fixtures/android-bionic.c']}}
    lib='/usr/lib/aarch64-linux-gnu';files={lib+'/'+name:sdk/('usr/lib/aarch64-linux-gnu/'+name)for name in ['libc.so.6','ld-linux-aarch64.so.1','libm.so.6']};files['/glibc-probe']=probe
    for file in sorted(locales.rglob('*')):
        if file.is_file():files['/usr/lib/locale/'+str(file.relative_to(locales))]=file
    imports=[x for guest,host in files.items()for x in ['--exec-file',guest+'='+str(host.resolve())]]
    with tempfile.TemporaryDirectory(prefix='glibc-stores-',dir=a.output)as temp:
        temp=Path(temp);store=temp/'persistent'
        def run(name,enabled,selection=bundle,extra=(),argument=None,expected=0,memory=False,cpus=4,seed=False):
            # The CLI deliberately creates exports exclusively, so each check
            # uses a fresh output name even when comparing the same library.
            for index,option in enumerate(extra):
                if option=='--export':Path(str(extra[index+1]).split('=',1)[1]).unlink(missing_ok=True)
            cmd=[str(a.runner.resolve()),*(['--memory']if memory else ['--storage',str(store)]),'--cpus',str(cpus),'--metrics','--max-instructions','4000000000','--env','LD_LIBRARY_PATH='+lib,'--env','LOCPATH=/usr/lib/locale','--accelerator','pure'if enabled else'off']
            if seed:
                overridden={str(extra[i+1]).split('=',1)[0]for i,option in enumerate(extra)if option in ('--file','--exec-file')}
                cmd += [part for guest,host in files.items() if guest not in overridden for part in ['--exec-file',guest+'='+str(host.resolve())]]
            cmd+=list(map(str,extra))
            if selection:cmd+=['--glibc-bundle',str(selection)]
            cmd+=['--guest',lib+'/ld-linux-aarch64.so.1','/glibc-probe']+([argument]if argument else[])
            done=subprocess.run(cmd,capture_output=True,text=True,timeout=120)
            (a.output/(name+'.stdout')).write_text(done.stdout);(a.output/(name+'.stderr')).write_text(done.stderr)
            if done.returncode!=expected:raise RuntimeError(f'{name}: exit {done.returncode}, expected {expected}: '+done.stdout[-1800:]+done.stderr[-2500:])
            metrics=next((json.loads(l.split(': ',1)[1])for l in done.stderr.splitlines()if l.startswith('trames native metrics: ')),{})
            result['cases'][name]={'exit':done.returncode,'native':metrics}
            return done.stdout,metrics
        reference,metrics=run('original',False,seed=True)
        assert metrics['calls']==0 and 'PASS glibc GNU aliases, C23, locale and exact FP status'in reference
        if a.oracle:assert reference==a.oracle.read_text(),'Original interpreter/QEMU mismatch'
        for cpus,memory in [(1,True),(4,True),(4,False)]:
            name=f'native-{cpus}-{memory}';actual,metrics=run(name,True,memory=memory,cpus=cpus,seed=memory)
            if actual!=reference:
                differences=[dict(original=x,native=y)for x,y in zip(reference.splitlines(),actual.splitlines())if x!=y];(a.output/'differences.json').write_text(json.dumps(differences,indent=2));raise RuntimeError('Original/native mismatch: '+str(differences[:5]))
            assert all(metrics['function_calls'][:30]) and all(metrics['glibc_operations']),metrics
            assert not any(metrics['glibc_fallbacks']) and not any(metrics['function_fallbacks']),metrics
            assert metrics['faults']==metrics['rejected']==0 and metrics['calls']==metrics['worker_calls'],metrics
        exported=temp/'exported.so'
        run('idempotent',True,extra=['--export',lib+'/libc.so.6='+str(exported)],argument='bench');assert exported.read_bytes()==(bundle/'libc-nativecall.so').read_bytes()
        text,_=run('preload',True,extra=['--exec-file','/interpose.so='+str(interpose),'--env','LD_PRELOAD=/interpose.so'],argument='preload');assert text=='PASS glibc interposition\n'
        upgraded=temp/'upgraded';upgraded.mkdir();tail=(bundle/'glibc-nativecall.bin').read_bytes()+bytes(4);upgrade=dict(manifest,added_file_bytes=len(tail),tail_sha256=hashlib.sha256(tail).hexdigest());(upgraded/'manifest.json').write_text(json.dumps(upgrade));(upgraded/'glibc-nativecall.bin').write_bytes(tail)
        run('variant-upgrade',True,upgraded,argument='bench');run('variant-downgrade',True,bundle,argument='bench')
        bad=temp/'bad';bad.mkdir();(bad/'manifest.json').write_bytes((bundle/'manifest.json').read_bytes());damaged=bytearray((bundle/'glibc-nativecall.bin').read_bytes());damaged[-1]^=1;(bad/'glibc-nativecall.bin').write_bytes(damaged)
        run('corrupt-resource',True,bad,expected=125,argument='bench')
        run('after-corrupt',True,None,extra=['--export',lib+'/libc.so.6='+str(exported)],argument='bench');assert exported.read_bytes()==(bundle/'libc-nativecall.so').read_bytes()
        run('restored',False,extra=['--export',lib+'/libc.so.6='+str(exported)],argument='bench');assert exported.read_bytes()==original.read_bytes()
        conflict=temp/'conflict';conflict.write_bytes(b'retained staging conflict')
        run('staging-conflict',True,extra=['--exec-file',lib+'/libc.so.6.trames-new='+str(conflict)],argument='bench',expected=125)
        run('cleanup',False,None,argument='cleanup-glibc');run('after-conflict',False,extra=['--export',lib+'/libc.so.6='+str(exported)],argument='bench');assert exported.read_bytes()==original.read_bytes()
        for kind,path in [('unknown-libc',original),('wrong-loader',sdk/'usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1')]:
            unknown=temp/(kind+'.so');data=bytearray(path.read_bytes());data[-1]^=1;unknown.write_bytes(data)
            old_store=store;store=temp/kind
            target=lib+('/libc.so.6'if kind=='unknown-libc'else'/ld-linux-aarch64.so.1')
            run(kind,True,extra=['--exec-file',target+'='+str(unknown)],argument='bench',expected=125,seed=True)
            assert 'unsupported' in (a.output/(kind+'.stderr')).read_text()
            run(kind+'-preserved',False,None,extra=['--export',lib+'/libc.so.6='+str(exported)],argument='bench')
            assert exported.read_bytes()==(bytes(data)if kind=='unknown-libc'else original.read_bytes())
            store=old_store
        for index in range(a.bench_rounds):
            for variant,enabled in [('original',False),('native',True)][::1 if index%2==0 else-1]:
                text,_=run(f'bench-{variant}-{index+1}',enabled,argument='bench')
                for line in text.splitlines():
                    if line.startswith('BENCH '):
                        _,name,size,count,ns=line.split();sample=result['benchmark'].setdefault(name,{}).setdefault(variant,dict(bytes=int(size),count=int(count),samples_ns=[]));sample['samples_ns'].append(int(ns))
    for variants in result['benchmark'].values():
        for sample in variants.values():sample['nanoseconds']=statistics.median(sample['samples_ns'])
        variants['speedup']=variants['original']['nanoseconds']/variants['native']['nanoseconds']
    (a.output/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS glibc exact original/native results, GNU ABI/locales/interposition, cold upgrade/restore/corruption, 1/4 CPU and memory/disk; '+str(len(result['benchmark']))+' benchmarks')

if __name__=='__main__':main()
