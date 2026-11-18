#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build the Debian 13 oracle, locale data and interposition regression fixture."""
import argparse,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'target/glibc-hybrid');p.add_argument('--cc',default=os.environ.get('AARCH64_CC','aarch64-linux-gnu-gcc'));p.add_argument('--qemu',default=os.environ.get('QEMU_AARCH64','qemu-aarch64'));a=p.parse_args();sdk=a.output/'sysroot';out=a.output/'fixtures';out.mkdir(exist_ok=True)
    flags=[a.cc,'--sysroot='+str(sdk),'-O2','-fno-builtin','-fno-tree-vectorize','-fno-tree-slp-vectorize','-Wall','-Wextra']
    subprocess.run(flags+[str(ROOT/'tests/fixtures/glibc.c'),'-Wl,--dynamic-linker,/usr/lib/aarch64-linux-gnu/ld-linux-aarch64.so.1','-pthread','-ldl','-lm','-o',str(out/'glibc-probe')],check=True)
    subprocess.run(flags+['-fPIC','-shared',str(ROOT/'tests/fixtures/glibc-preload.c'),'-ldl','-o',str(out/'interpose.so')],check=True)
    source=a.output/'source/glibc-2.41/localedata';locale=sdk/'usr/lib/locale';locale.mkdir(exist_ok=True)
    for name,charmap in [('tr_TR','ISO-8859-9'),('fr_FR','UTF-8')]:
        target=locale/(name+'.'+charmap)
        if not(target/'LC_CTYPE').exists():
            subprocess.run([a.qemu,'-cpu','max','-L',str(sdk),'-E','LC_ALL=C','-E','I18NPATH='+str(source),str(sdk/'usr/bin/localedef'),'--no-archive','-i',str(source/'locales'/name),'-f',str(source/'charmaps'/charmap),str(target)],check=True)
    print('Built glibc complete/ABI probe, preload fixture and real Turkish/French locales')
