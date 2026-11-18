#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fetch SHA-pinned Debian 13 validation inputs; normal adapter builds are offline."""
import argparse,hashlib,io,json,shutil,subprocess,tarfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES={
 'libc6_2.41-12+deb13u4_arm64.deb':'8784eda966b189c777a384dac5ce009e8fc9b52d006926c5a013e7fa8aa688cc',
 'libc6-dev_2.41-12+deb13u4_arm64.deb':'71e373cb5e9da975b02fb4af555929cb1977560e2dd8245de4eb8a7b03677a08',
 'libc-bin_2.41-12+deb13u4_arm64.deb':'3aba9182d6989d7c512178defdcb9d02b6906816bc8b2778a82a6c39582c880c',
 'glibc_2.41.orig.tar.xz':'f24aa441021121a79266f0d75242706cab8843a47901fefe74527491807f1998'}
def unpack(archive,destination,members=None):
    destination.mkdir(parents=True,exist_ok=True)
    # Every archive is immutable and digest checked, with paths still confined
    # to this disposable build root. This does not extract into the host /usr.
    selected=members if members is not None else archive.getmembers()
    selected=[m for m in selected if not m.name.startswith('/') and '..' not in Path(m.name).parts]
    archive.extractall(destination,members=selected,filter='fully_trusted') if hasattr(tarfile,'data_filter') else archive.extractall(destination,members=selected)
def prepare(output,kernel_headers):
    cache=output/'downloads';cache.mkdir(parents=True,exist_ok=True);sysroot=output/'sysroot';sysroot.mkdir(exist_ok=True)
    for name,digest in FILES.items():
        file=cache/name
        if not file.exists():subprocess.run(['curl','--fail','--location','--silent','--show-error','--max-time','120','--retry','2','https://deb.debian.org/debian/pool/main/g/glibc/'+name,'-o',str(file)],check=True)
        if hashlib.sha256(file.read_bytes()).hexdigest()!=digest:raise ValueError('glibc input digest mismatch: '+name)
        if name.endswith('.deb'):
            raw=file.read_bytes();at=8
            while at<len(raw):
                h=raw[at:at+60];size=int(h[48:58]);member=h[:16].strip().rstrip(b'/');body=raw[at+60:at+60+size];at+=60+size+(size%2)
                if member.startswith(b'data.tar'):
                    with tarfile.open(fileobj=io.BytesIO(body))as archive:unpack(archive,sysroot)
                    break
        elif not (output/'source/glibc-2.41/localedata').exists():
            with tarfile.open(file)as archive:unpack(archive,output/'source',[m for m in archive if '/localedata/'in m.name or m.name.endswith('/COPYING.LIB')])
    for name in ['linux','asm-generic']:
        link=sysroot/'usr/include'/name
        if not link.exists():link.symlink_to(kernel_headers/name)
    asm=sysroot/'usr/include/aarch64-linux-gnu/asm'
    if not asm.exists():asm.symlink_to(kernel_headers/'asm'if(kernel_headers/'asm').exists()else Path('/usr/include/aarch64-linux-gnu/asm'))
    if not(sysroot/'lib').exists():(sysroot/'lib').symlink_to('usr/lib')
    (output/'inputs.json').write_text(json.dumps(FILES,indent=2)+'\n')
    return sysroot
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'target/glibc-hybrid');p.add_argument('--kernel-headers',type=Path,default=Path('/usr/aarch64-linux-gnu/include')if Path('/usr/aarch64-linux-gnu/include/asm').exists()else Path('/usr/include'));a=p.parse_args();print(prepare(a.output,a.kernel_headers))
