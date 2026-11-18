#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a cold-selected, pinned Debian 13 libc derivative; no preload or AOSP.

The small distributable contains precompiled guest code plus a reversible recipe.
The original loader, symbols, versions, relocations, TLS, locale and allocator
remain intact. --libc additionally exports the two complete validation variants.
"""
import argparse,hashlib,importlib.util,json,os,shutil,struct,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('elflib',ROOT/'scripts/build-bionic-hybrid.py');elflib=importlib.util.module_from_spec(spec);spec.loader.exec_module(elflib)
ORIGINAL_SHA256='70fa4c4cf57eabeb96097cce47582d02379c4cd1de3db3e98ade4dd7089a1b13'
LOADER_SHA256='1b882eddacd494b060413a8aa4f0a4754ee3f84775e68cecbf8a86092b59cca6'
VERSION='2.41-12+deb13u4'
def build(original,output,cc='clang'):
 layout=json.loads((ROOT/'accelerator/glibc/layout.json').read_text());assert layout['original_sha256']==ORIGINAL_SHA256 and layout['loader_sha256']==LOADER_SHA256
 raw=original.read_bytes() if original else None
 if raw is not None and hashlib.sha256(raw).hexdigest()!=ORIGINAL_SHA256:raise ValueError('unsupported Debian libc; retained unchanged')
 output.mkdir(parents=True,exist_ok=True);ph=list(layout['program_headers'])
 assert ph[0][0]==6 and ph[0][2]==64 and ph[0][5]==len(ph)*56,'unexpected pinned PHDR layout'
 offset=elflib.align(layout['original_size']);address=elflib.align(max(p[3]+p[6]for p in ph if p[0]==1));text=address+elflib.align((len(ph)+1)*56,16)
 entries=sorted({s['site']for s in layout['functions'].values()if not s['ifunc']});trampolines={entry:text+i*16 for i,entry in enumerate(entries)};words=[]
 for entry in entries:
  s=next(s for s in layout['functions'].values()if s['site']==entry)
  relocated=elflib.displaced(int.from_bytes(bytes.fromhex(s['original']),'little'),entry,trampolines[entry]);assert len(relocated)<=4
  words+=relocated+[0xd503201f]*(4-len(relocated))
 wrapper_address=text+len(words)*4
 rust=Path(subprocess.check_output(['rustc','--print','sysroot'],text=True).strip());host=next(x.split(': ',1)[1]for x in subprocess.check_output(['rustc','-vV'],text=True).splitlines()if x.startswith('host: '));linker=rust/'lib/rustlib'/host/'bin/rust-lld'
 flags=[cc,'--target=aarch64-linux-gnu','-O2','-ffreestanding','-fPIC','-fno-builtin','-fno-stack-protector','-fno-unwind-tables','-fno-asynchronous-unwind-tables','-mno-outline-atomics','-mbranch-protection=bti','-fvisibility=hidden','-Wall','-Wextra','-Werror']
 with tempfile.TemporaryDirectory(dir=output,prefix='build-')as tmp:
  tmp=Path(tmp)
  for source,name in [('accelerator/glibc/glibc.c','wrappers'),('accelerator/glibc/calls.S','glibc-calls'),('accelerator/guest/calls.S','pure-calls')]:subprocess.run(flags+['-c',str(ROOT/source),'-o',str(tmp/(name+'.o'))],check=True)
  script=tmp/'native.ld';script.write_text(f'SECTIONS {{ . = {wrapper_address}; .text : {{ *(.text*) *(.rodata*) }} .data : {{ *(.data*) *(.bss*) }} /DISCARD/ : {{ *(.comment) *(.note*) *(.eh_frame*) }} }}\nASSERT(SIZEOF(.data)==0,"glibc thunks must have no writable state")\n')
  command=[str(linker),'-flavor','gnu','-m','aarch64elf','--no-undefined','--build-id=none','--entry=trames_glibc_memcpy','-T',str(script)]
  command += [f'--defsym=trames_orig_{name}={s["fallback"]if s["ifunc"]else trampolines[s["site"]]}'for name,s in layout['functions'].items()]
  command += [f'--defsym={name}={value}'for name,value in layout['references'].items()]
  command += [str(tmp/(name+'.o'))for name in ['wrappers','glibc-calls','pure-calls']]+['-o',str(tmp/'payload.so')]
  subprocess.run(command,check=True);payload=elflib.Elf((tmp/'payload.so').read_bytes());symbols=payload.symbols();section=payload.sections['.text'];assert section[3]==wrapper_address
  assert not any(s[5] for name,s in payload.sections.items() if name.startswith(('.rel','.got','.dynamic'))),'payload must be fully statically linked'
  assert all(not(s[2]&2) or s[5]==0 or name=='.text' for name,s in payload.sections.items()),'unpackaged allocated payload section'
  code=struct.pack('<'+'I'*len(words),*words)+payload.section_bytes(section)
 patches=[];seen=set()
 for name,s in layout['functions'].items():
  if s['site']in seen:continue
  seen.add(s['site']);target=symbols['trames_glibc_'+s['wrapper']]['address']
  if s['ifunc']:
   delta=(target>>12)-(s['site']>>12);assert -(1<<20)<=delta<(1<<20)
   replacement=struct.pack('<3I',0x90000000|((delta&3)<<29)|(((delta>>2)&0x7ffff)<<5),0x91000000|((target&4095)<<10),0xd65f03c0)
  else:replacement=struct.pack('<I',elflib.branch(s['site'],target))
  patches.append(dict(name=name,offset=s['offset'],original=s['original'],nativecall=replacement.hex()))
 # Keep every original load segment and dynamic table. Move only the PHDR
 # array to the added RX allocation and point the original PT_PHDR at it.
 count=len(ph)+1;segment_size=text-address+len(code)
 for i,p in enumerate(ph):
  if p[0]==6:ph[i]=(6,4,offset,address,address,count*56,count*56,8)
 ph.append((1,5,offset,address,address,segment_size,segment_size,4096))
 tail=bytes(offset-layout['original_size'])+b''.join(struct.pack('<IIQQQQQQ',*p)for p in ph)+bytes(text-address-len(ph)*56)+code
 # Header mutations participate in the same reversible patch recipe.
 for name,at,value in [('phoff',32,struct.pack('<Q',offset)),('phnum',56,struct.pack('<H',len(ph)))]:
  original_header=struct.pack('<Q',64)if name=='phoff'else struct.pack('<H',len(layout['program_headers']))
  patches.append(dict(name=name,offset=at,original=original_header.hex(),nativecall=value.hex()))
 inputs={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()for p in [ROOT/'scripts/build-glibc-hybrid.py',ROOT/'scripts/build-bionic-hybrid.py',ROOT/'accelerator/glibc/layout.json',ROOT/'accelerator/glibc/glibc.c',ROOT/'accelerator/glibc/calls.S',ROOT/'accelerator/guest/calls.S']}
 manifest=dict(schema=1,kind='glibc-static',version=VERSION,abi='arm64-v8a',original_sha256=ORIGINAL_SHA256,loader_sha256=LOADER_SHA256,original_size=layout['original_size'],native_profile=65537,glibc_profile=65546,build_id=layout['build_id'],functions=list(layout['functions']),tail_sha256=hashlib.sha256(tail).hexdigest(),added_file_bytes=len(tail),added_code_bytes=len(code),patches=patches,inputs_sha256=inputs,upstream=layout['upstream'])
 (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');(output/'glibc-nativecall.bin').write_bytes(tail);shutil.copyfile(ROOT/'LICENSE',output/'GPL-3.0.txt')
 if raw is not None:
  native=bytearray(raw)
  for p in patches:
   start=p['offset'];before=bytes.fromhex(p['original']);assert native[start:start+len(before)]==before,p['name'];native[start:start+len(before)]=bytes.fromhex(p['nativecall'])
  native.extend(tail);(output/'libc-nativecall.so').write_bytes(native);(output/'libc-original.so').write_bytes(raw)
 return manifest
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--libc',type=Path);p.add_argument('--output',type=Path,default=ROOT/'target/glibc-hybrid/bundle');p.add_argument('--cc',default=os.environ.get('CC','clang'));a=p.parse_args();m=build(a.libc,a.output,a.cc);print(json.dumps({k:m[k]for k in ['version','functions','added_code_bytes','tail_sha256']},indent=2))
