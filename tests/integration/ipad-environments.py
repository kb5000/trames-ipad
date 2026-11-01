#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""iPad production FFI: pinned Debian, custom rootfs and optional systemd PTYs."""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import re
import shutil
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--ffi',type=Path,required=True)
p.add_argument('--alpine',type=Path,required=True)
p.add_argument('--debian',type=Path,required=True)
p.add_argument('--musl',type=Path,required=True)
p.add_argument('--prepared-stores',type=Path,help='reuse previously verified cold imports')
p.add_argument('--guest-vulkan',type=Path,help='bundled iPad guest adapters')
p.add_argument('--systemd-store',type=Path,help='stopped disposable Debian store with original systemd/dbus installed')
p.add_argument('--output',type=Path,required=True)
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
api=C.CDLL(str(a.ffi.resolve()))
api.trames_environment_install.argtypes=[C.c_char_p,C.c_uint32]
api.trames_session_open.argtypes=[C.c_char_p,C.c_uint32];api.trames_session_open.restype=C.c_uint64
api.trames_session_close.argtypes=[C.c_uint64]
api.trames_session_terminal_open.argtypes=[C.c_uint64];api.trames_session_terminal_open.restype=C.c_uint64
api.trames_session_terminal_input.argtypes=[C.c_uint64,C.c_uint64,C.c_char_p,C.c_uint32]
api.trames_session_terminal_output.argtypes=[C.c_uint64,C.c_uint64,C.c_void_p,C.c_uint32]
api.trames_session_status.argtypes=[C.c_uint64,C.c_void_p,C.c_uint32]
api.trames_last_error.argtypes=[C.c_void_p,C.c_uint32]
checks=[];h=0;transcripts={};name=''
def last_error():
 b=C.create_string_buffer(8192);n=api.trames_last_error(b,len(b));return b.raw[:max(0,n)].decode(errors='replace')
def call_json(method,options):
 b=json.dumps(options).encode();return getattr(api,method)(b,len(b))
def config(store,distro):
 options=dict(store=str(store.resolve()),distro=distro,volume_total=16<<30,volume_available=16<<30,memory_budget_bytes=4<<30,process_memory_budget_bytes=4<<30,pure_accelerator=True,network='off',wayland=True,multiple_terminals=True)
 if a.guest_vulkan:options.update(vulkan_accelerator=True,vulkan_icd=str((a.guest_vulkan/'libtrames_vulkan_icd.so').resolve()),xwayland_launcher=str((a.guest_vulkan/'trames-xwayland').resolve()))
 return options
def install(case,distro,archive,**extra):
 options=config((a.prepared_stores or a.output)/case,distro);options.update(rootfs=str(archive.resolve()),**extra)
 if not a.prepared_stores:assert call_json('trames_environment_install',options)==0,last_error()
 else:assert (Path(options['store'])/'manifest').is_file()
 checks.append(dict(case=case+'-install',passed=True,reused=bool(a.prepared_stores)));print('PASS',case,'install',flush=True);return options
def read(method,*ids):
 b=C.create_string_buffer(65536);n=getattr(api,method)(h,*ids,b,len(b));assert n>=0,last_error();return b.raw[:n]
def pump(terminal):
 chunk=read('trames_session_terminal_output',terminal)
 transcripts[terminal]=transcripts.get(terminal,b'')+chunk
 if chunk:
  with (a.output/f'{name}-terminal-{terminal}.log').open('ab') as f:f.write(chunk)
 status=json.loads(read('trames_session_status'))
 assert status['phase'] in ('starting','running'),status
 return status
def send(terminal,text):
 b=text.encode();assert api.trames_session_terminal_input(h,terminal,b,len(b))==len(b),last_error()
def until(terminal,needle,offset=0,timeout=90):
 deadline=time.monotonic()+timeout
 while True:
  pump(terminal)
  if needle in transcripts[terminal][offset:]:return
  assert time.monotonic()<deadline,(name,needle,transcripts[terminal][-1500:])
  time.sleep(.01)
def start(case,options):
 global h,name,transcripts
 name=case;transcripts={};h=call_json('trames_session_open',options);assert h,last_error()
def close():
 global h
 if h:api.trames_session_close(h);h=0;time.sleep(.15)
def shell_checks(case,options,systemd=False):
 start(case,options)
 try:
  until(1,b'# ')
  for terminal in [1,2]:
   if terminal==2:assert api.trames_session_terminal_open(h)==2
   until(terminal,b'# ')
   offset=len(transcripts[terminal]);marker=f'{case.upper()}_{terminal}_OK'
   assertion='set -e; test "$(cat /proc/1/comm)" = systemd; systemctl is-active --quiet dbus.service; ' if systemd else ''
   send(terminal,f"{assertion}printf '\\n{marker}\\n'\r")
   until(terminal,('\r\n'+marker+'\r\n').encode(),offset)
   checks.append(dict(case=case,terminal=terminal,passed=True));print('PASS',case,'terminal',terminal,flush=True)
 finally:close()
try:
 alpine=install('alpine','alpine',a.alpine,musl_libc=str(a.musl.resolve()))
 shell_checks('alpine',alpine)
 debian=install('debian','debian',a.debian)
 shell_checks('debian',debian)
 setup=dict(debian,multiple_terminals=False,debian_setup=dict(packages='minimal',systemd=False))
 start('debian-setup',setup)
 try:
  deadline=time.monotonic()+30
  while True:
   chunk=read('trames_session_terminal_output',1)
   with (a.output/'debian-setup.log').open('ab') as f:f.write(chunk)
   status=json.loads(read('trames_session_status'))
   if status['phase']=='exited':assert status['exit_code']==0,status;break
   assert status['phase'] in ('starting','running') and time.monotonic()<deadline,status
   time.sleep(.01)
  checks.append(dict(case='debian-setup',passed=True))
 finally:close()
 custom=install('custom','custom',a.debian,rootfs_sha256=hashlib.sha256(a.debian.read_bytes()).hexdigest())
 shell_checks('custom',custom)
 bad=config(a.output/'bad-digest','custom');bad.update(rootfs=str(a.debian.resolve()),rootfs_sha256='0'*64)
 assert call_json('trames_environment_install',bad)==-1
 assert 'SHA-256 mismatch' in last_error(),last_error()
 checks.append(dict(case='custom-sha256-rejection',passed=True))
 if a.systemd_store:
  destination=a.output/'systemd';shutil.copytree(a.systemd_store,destination)
  options=config(destination,'debian');options['init']='systemd'
  shell_checks('systemd',options,True)
finally:
 close();(a.output/'results.json').write_text(json.dumps(dict(passed=len(checks)==(13 if a.systemd_store else 11),checks=checks),indent=2)+'\n')
