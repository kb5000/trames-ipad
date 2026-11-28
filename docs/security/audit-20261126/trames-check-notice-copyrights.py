from pathlib import Path
import json,tarfile,zipfile,io,re
out=Path('/Users/main/dev/trames/docs/security/audit-20261126');cache=Path('/private/tmp/trames-license-audit-20261126/releases');needles={'generic-array':'Kamiński','simd-adler32':'Marvin Countryman','libloading':'Simonas Kazlauskas'}
def textscan(data,depth=0):
 blob=[]
 with tarfile.open(fileobj=io.BytesIO(data),mode='r:*')as tf:
  for m in tf:
   if not m.isfile()or Path(m.name).name.startswith('._'):continue
   if m.name.endswith(('.tar.gz','.tgz','.tar.xz'))and depth<2 and m.size<150000000:
    try:blob+=textscan(tf.extractfile(m).read(),depth+1)
    except:pass
   elif m.size<2000000:
    b=tf.extractfile(m).read()
    if b'\0'not in b[:1000]:blob.append(b.decode(errors='replace'))
 return blob
rows=[]
for repo in ['trames-ipad','trames-mac']:
 for folder in sorted((cache/repo).iterdir()):
  sources=[]
  for p in folder.glob('*.tar.gz'):sources+=textscan(p.read_bytes())
  for p in list(folder.glob('*.ipa'))+list(folder.glob('*.zip')):
   chunks=sources[:];binarymarkers=set();locks=[]
   with zipfile.ZipFile(p)as z:
    for i in z.infolist():
     if i.is_dir()or Path(i.filename).name.startswith('._'):continue
     if i.filename.endswith('.tar.gz'):
      try:chunks+=textscan(z.read(i))
      except:pass
     elif i.file_size<2000000:
      b=z.read(i)
      if b'\0'not in b[:1000]:chunks.append(b.decode(errors='replace'))
     if Path(i.filename).name in ['Trames','trames-mac-run']:
      b=z.read(i)
      for name in needles:
       if name.replace('-','_').encode()in b:binarymarkers.add(name)
   text='\n'.join(chunks)
   rows.append({'repo':repo,'tag':folder.name,'asset':p.name,'searched_binary_source_and_text_files':len(chunks),'copyright_present':{name:token in text for name,token in needles.items()},'binary_symbol_strings':sorted(binarymarkers)})
(out/'release-copyright-checks.json').write_text(json.dumps(rows,indent=2)+'\n')
for x in rows:print(x['repo'],x['tag'],x['asset'],x['copyright_present'],x['binary_symbol_strings'])
