from pathlib import Path
import json,struct,re,subprocess,hashlib
r=Path('/Users/main/dev/trames');out=r/'docs/security/audit-20261126';a=[]
for base in [r/'apps/ios/Trames/GuestMusl',r/'apps/ios/Trames/GuestVulkan',r/'apps/ios/Trames/GuestVideo']:
 for p in base.glob('*'):
  if not p.is_file() or p.suffix in ['.apk','.tar','.json']:continue
  b=p.read_bytes()
  if b[:6]!=b'\x7fELF\x02\x01':continue
  try:
   at=struct.unpack_from('<Q',b,40)[0];sz,cnt,nidx=struct.unpack_from('<HHH',b,58)
   sh=[struct.unpack_from('<IIQQQQIIQQ',b,at+i*sz)for i in range(cnt)]
   names=b[sh[nidx][4]:sh[nidx][4]+sh[nidx][5]]
   sections={names[x[0]:].split(b'\0',1)[0].decode(errors='replace'):x for x in sh}
   needed=[]
   if '.dynamic'in sections:
    d=sections['.dynamic'];t=sections['.dynstr'];ss=b[t[4]:t[4]+t[5]]
    for pos in range(d[4],d[4]+d[5],16):
     tag,value=struct.unpack_from('<QQ',b,pos)
     if tag==1:needed.append(ss[value:].split(b'\0',1)[0].decode())
   candidates=[x.decode(errors='replace')for x in re.findall(rb'[\x20-\x7e]{10,}',b)if b'--enable-gpl'in x or b'--enable-nonfree'in x or b'--disable-autodetect'in x or b'LGPL version'in x or b'GPL version'in x]
   a.append({'path':str(p),'sha256':hashlib.sha256(b).hexdigest(),'needed':needed,'configuration_or_license_strings':candidates})
  except Exception as e:a.append({'path':str(p),'error':str(e)})
(out/'staged-elf-dependencies.json').write_text(json.dumps(a,indent=2)+'\n')
print('Staged ELF files',len(a))
for x in a:
 if 'libav'in x['path']:print(Path(x['path']).name,x.get('needed'),x.get('configuration_or_license_strings'))
