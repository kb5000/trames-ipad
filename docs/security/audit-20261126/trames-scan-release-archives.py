from pathlib import Path
import zipfile,tarfile,io,json,re,hashlib,collections,sys
sys.path.insert(0,"/private/tmp/trames-license-audit-20261126/tools")
import tomli
out=Path('/Users/main/dev/trames/docs/security/audit-20261126');cache=Path('/private/tmp/trames-license-audit-20261126/releases')
records=[];texts=out/'license-texts';texts.mkdir(exist_ok=True)
def noteworthy(n):return bool(re.search(r'(?i)license|licence|copying|copyright|notice|unlicense|(?:^|[-/])(?:L?GPL)[-_.]', n))
def tar_index(data,label,depth=0):
 results={'archive':label,'members':[],'license_texts':[],'nested_archives':[],'cargo_locks':[],'cargo_manifests':[]}
 with tarfile.open(fileobj=io.BytesIO(data),mode='r:*')as tf:
  for m in tf:
   if not m.isfile() or Path(m.name).name.startswith('._'):continue
   results['members'].append({'path':m.name,'bytes':m.size})
   if m.name.endswith('Cargo.lock') and m.size<2000000:
    try:results['cargo_locks'].append({'path':m.name,'packages':tomli.loads(tf.extractfile(m).read().decode()).get('package',[])})
    except Exception as e:results['cargo_locks'].append({'path':m.name,'error':str(e)})
   if m.name.endswith('Cargo.toml') and m.size<200000:
    body=tf.extractfile(m).read().decode(errors='replace')
    if 'cli/crates/trames-cli-common' in body or 'trames-cli-common' in body:results['cargo_manifests'].append({'path':m.name,'references_cli_common':True})

   if noteworthy(m.name)and m.size<2000000:
    content=tf.extractfile(m).read();sha=hashlib.sha256(content).hexdigest();(texts/(sha+'.txt')).write_bytes(content)
    results['license_texts'].append({'path':m.name,'sha256':sha,'text_file':'license-texts/'+sha+'.txt'})
   if depth<2 and m.name.endswith(('.tar.gz','.tgz','.tar.xz')) and m.size<150000000:
    try:results['nested_archives'].append(tar_index(tf.extractfile(m).read(),label+'!'+m.name,depth+1))
    except Exception as e:results['nested_archives'].append({'archive':m.name,'error':str(e)})
 return results
for p in sorted(cache.rglob('*')):
 if not p.is_file()or not p.name.endswith(('.ipa','.zip','.tar.gz','.tgz')):continue
 rec={'path':str(p),'repository':p.parent.parent.name,'tag':p.parent.name,'name':p.name}
 try:
  if p.suffix in ['.ipa','.zip']:
   with zipfile.ZipFile(p)as z:
    files=[i for i in z.infolist()if not i.is_dir() and not Path(i.filename).name.startswith('._')];rec['members']=[{'path':i.filename,'bytes':i.file_size}for i in files];rec['license_texts']=[];rec['source_archives']=[];rec['binary_markers']=[]
    for i in files:
     name=i.filename
     if noteworthy(name)and i.file_size<2000000:
      b=z.read(i);sha=hashlib.sha256(b).hexdigest();(texts/(sha+'.txt')).write_bytes(b);rec['license_texts'].append({'path':name,'sha256':sha,'text_file':'license-texts/'+sha+'.txt'})
     if name.endswith(('.tar.gz','.tgz'))and i.file_size<150000000:
      rec['source_archives'].append(tar_index(z.read(i),p.name+'!'+name))
     if name.endswith(('libavcodec.so.61','libavutil.so.59','libswresample.so.5','libswscale.so.8')):
      b=z.read(i);rec['binary_markers'].append({'path':name,'sha256':hashlib.sha256(b).hexdigest(),'license_strings':[x.decode(errors='replace')for x in re.findall(rb'[\x20-\x7e]{12,}',b)if b'license: 'in x or b'--enable-gpl'in x or b'--enable-nonfree'in x or b'--disable-autodetect'in x]})
    rec['contains_ffmpeg']=any('libavcodec' in i.filename for i in files)
    rec['contains_cli_sources']=any('/trames-cli/'in i.filename or i.filename.endswith('/trames-cli.tar.gz')for i in files)
  else:rec['source_tree']=tar_index(p.read_bytes(),p.name)
 except Exception as e:rec['error']=str(e)
 records.append(rec);print('Scanned',rec['repository'],rec['tag'],rec['name'],flush=True)
(out/'release-archive-scans.json').write_text(json.dumps(records,indent=2)+'\n')
print('COMPLETE scanned archives',len(records),'errors',[(x['name'],x.get('error'))for x in records if x.get('error')],flush=True)
