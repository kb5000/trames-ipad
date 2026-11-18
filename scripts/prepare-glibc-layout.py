#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Maintainer tool: record an exact Debian libc/debug pair; never patch a process."""
import argparse,hashlib,importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('bionic',ROOT/'scripts/build-bionic-hybrid.py');elflib=importlib.util.module_from_spec(spec);spec.loader.exec_module(elflib)
VERSION='2.41-12+deb13u4'
ORIGINAL='70fa4c4cf57eabeb96097cce47582d02379c4cd1de3db3e98ade4dd7089a1b13'
LOADER='1b882eddacd494b060413a8aa4f0a4754ee3f84775e68cecbf8a86092b59cca6'
FUNCTIONS=list(elflib.FUNCTIONS)
FUNCTIONS+='strcasecmp_l strncasecmp_l strtol_l strtoll_l strtoul_l strtoull_l strtod_l strtof_l __isoc23_strtol __isoc23_strtoll __isoc23_strtoul __isoc23_strtoull __isoc23_strtoimax __isoc23_strtoumax __isoc23_strtol_l __isoc23_strtoll_l __isoc23_strtoul_l __isoc23_strtoull_l __strtol_internal __strtoll_internal __strtoul_internal __strtoull_internal __strtod_internal __strtof_internal'.split()
CANONICAL={'strtoll':'strtol','strtoimax':'strtol','strtoull':'strtoul','strtoumax':'strtoul','strtoll_l':'strtol_l','strtoull_l':'strtoul_l','__isoc23_strtoll':'__isoc23_strtol','__isoc23_strtoimax':'__isoc23_strtol','__isoc23_strtoull':'__isoc23_strtoul','__isoc23_strtoumax':'__isoc23_strtoul','__isoc23_strtoll_l':'__isoc23_strtol_l','__isoc23_strtoull_l':'__isoc23_strtoul_l','__strtoll_internal':'__strtol_internal','__strtoull_internal':'__strtoul_internal'}
DEFAULTS={'memcpy':'__memcpy_generic','memmove':'__memmove_generic','memset':'__memset_generic','memchr':'__memchr_generic','strlen':'__strlen_asimd'}
def build(libc,debugroot,loader,output):
 raw=libc.read_bytes();assert hashlib.sha256(raw).hexdigest()==ORIGINAL
 assert hashlib.sha256(loader.read_bytes()).hexdigest()==LOADER
 elf=elflib.Elf(raw);note=elf.section_bytes(elf.sections['.note.gnu.build-id']);size,desc,_=struct.unpack_from('<III',note);at=12+((size+3)&~3);buildid=note[at:at+desc].hex()
 debug=debugroot/buildid[:2]/(buildid[2:]+'.debug');symbols=elflib.Elf(debug.read_bytes()).symbols();exports=elf.symbols('.dynsym');functions={}
 for name in FUNCTIONS:
  # Some aliases are versioned exports but not standalone dynsym names.
  if name not in exports:continue
  s=exports[name];address=s['address'];bti=raw[elf.offset(address):elf.offset(address)+4]==bytes.fromhex('5f2403d5');site=address+(4 if bti else 0)
  size=12 if s['kind']==10 else 4
  assert site+size<=address+s['size'],name
  functions[name]={'entry':address,'site':site,'offset':elf.offset(site,size),'original':raw[elf.offset(site,size):elf.offset(site,size)+size].hex(),'ifunc':s['kind']==10,'fallback':symbols[DEFAULTS[name]]['address']if name in DEFAULTS else site,'wrapper':CANONICAL.get(name,name)}
 refs={'trames_errno_location':symbols['__errno_location']['address'],'trames_ctype_tolower_loc':symbols['__ctype_tolower_loc']['address'],'trames_uselocale':symbols['uselocale']['address'],'trames_C_tolower':symbols['_nl_C_LC_CTYPE_tolower']['address']+128*4,'trames_global_locale':symbols['_nl_global_locale']['address']}
 record={'schema':1,'version':VERSION,'original_sha256':ORIGINAL,'loader_sha256':LOADER,'original_size':len(raw),'build_id':buildid,'debug_sha256':hashlib.sha256(debug.read_bytes()).hexdigest(),'program_headers':elf.ph,'functions':functions,'references':refs,'upstream':{'libc_package_sha256':'8784eda966b189c777a384dac5ce009e8fc9b52d006926c5a013e7fa8aa688cc','debug_package_sha256':'baddf9f4195707f5cf74b32da8b215d4fc808d5a4825c2c0e72d8168e7434143','source_sha256':'f24aa441021121a79266f0d75242706cab8843a47901fefe74527491807f1998','debian_patch_sha256':'dda4153511bfd543502d18e5bc9323110996fe9c531f14ae3fad6eb17f027c7d'}}
 output.write_text(json.dumps(record,indent=2)+'\n');print('Recorded',len(functions),'public/versioned entries; exact original, loader and debug build ID')
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--libc',type=Path,required=True);p.add_argument('--loader',type=Path,required=True);p.add_argument('--debug-root',type=Path,required=True);p.add_argument('--output',type=Path,default=ROOT/'accelerator/glibc/layout.json');a=p.parse_args();build(a.libc,a.debug_root,a.loader,a.output)
