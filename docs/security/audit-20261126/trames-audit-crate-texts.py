from pathlib import Path
import json,sys,tarfile,io,hashlib,urllib.request,concurrent.futures,re,subprocess
sys.path.insert(0,'/private/tmp/trames-license-audit-20261126/tools');import tomli
out=Path('/Users/main/dev/trames/docs/security/audit-20261126');cache=Path('/private/tmp/trames-license-audit-20261126/crates');cache.mkdir(exist_ok=True);texts=out/'license-texts';texts.mkdir(exist_ok=True)
a=json.loads((out/'rust-dependencies.json').read_text())+json.loads((out/'auxiliary-rust-dependencies.json').read_text());unique={}
for x in a:unique.setdefault((x['name'],x['version'],x['source']),x)
checks={}
for p in [Path('/Users/main/dev/trames/Cargo.lock'),Path('/Users/main/dev/trames/runtime/Cargo.lock'),Path('/Users/main/dev/trames-mac/Cargo.lock'),Path('/Users/main/dev/trames-linux/Cargo.lock'),Path('/Users/main/dev/trames-cli/Cargo.lock'),Path('/Users/main/dev/trames/tests/performance/boundary-cost/Cargo.lock'),Path('/Users/main/dev/trames/runtime/vendor/wayland-backend/Cargo.lock'),Path('/Users/main/dev/trames-mac/benchmarks/memory-architecture/Cargo.lock')]:
 for x in tomli.loads(p.read_text())['package']:
  if x.get('checksum'):checks[(x['name'],x['version'])]=x['checksum']
def save(data,label):
 sha=hashlib.sha256(data).hexdigest();p=texts/(sha+'.txt')
 if not p.exists():p.write_bytes(data)
 return {'origin':label,'sha256':sha,'text_file':'license-texts/'+p.name,'bytes':len(data)}
def get(x):
 n,v=x['name'],x['version'];record={'name':n,'version':v,'source':x['source'],'declared_license':x['license'],'license_texts':[]}
 try:
  if x['source']=='vendored-path':
   for f in x.get('evidence',[]):record['license_texts'].append(save(Path(f['path']).read_bytes(),f['path']))
   record['checksum_status']='vendored evidence hashes recorded'
   return record
  cached=list(Path('/Users/main/.cargo/registry/cache').glob('*/'+n+'-'+v+'.crate'))
  p=cached[0]if cached else cache/(n+'-'+v+'.crate')
  url=f'https://static.crates.io/crates/{n}/{n}-{v}.crate'
  if not p.exists():p.write_bytes(urllib.request.urlopen(url,timeout=45).read())
  data=p.read_bytes();assert hashlib.sha256(data).hexdigest()==checks[(n,v)],'Cargo.lock checksum mismatch'
  record['checksum_status']='verified against Cargo.lock';record['archive_url']=url
  with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz')as tf:
   vcs=None;repo=x.get('repository');manifest=None
   for m in tf:
    if not m.isfile():continue
    name=m.name.split('/',1)[-1]
    if name=='.cargo_vcs_info.json':vcs=json.load(tf.extractfile(m))
    if name=='Cargo.toml':manifest=tomli.loads(tf.extractfile(m).read().decode())['package'];repo=repo or manifest.get('repository')
    if re.search(r'(?i)(?:^|/)(?:licen[sc]e[^/]*|copying[^/]*|copyright[^/]*|notice[^/]*|unlicense)$',name):
     record['license_texts'].append(save(tf.extractfile(m).read(),url+'!'+name))
   record['repository']=repo
   record['vcs']=vcs
  # Recover omitted workspace-root license texts from the exact original commit.
  if not record['license_texts'] and vcs and repo and 'github.com/'in repo:
   repository=repo.split('github.com/',1)[1].removesuffix('.git').strip('/');sha=vcs['git']['sha1']
   result=subprocess.run(['gh','api',f'repos/{repository}/git/trees/{sha}?recursive=1'],capture_output=True,text=True)
   if result.returncode:
    record['supplement_error']=result.stderr[-250:]
   else:
    tree=json.loads(result.stdout)['tree'];relative=vcs.get('path_in_vcs','');ancestors={'',relative}
    path=Path(relative)
    ancestors.update(''if str(p)=='.'else str(p)for p in path.parents)
    candidates=[t['path']for t in tree if t['type']=='blob'and (str(Path(t['path']).parent)if str(Path(t['path']).parent)!='.'else'')in ancestors and re.match(r'(?i)^(LICENSE|COPYING|COPYRIGHT|NOTICE|UNLICENSE)',Path(t['path']).name)]
    for name in candidates:
     raw=f'https://raw.githubusercontent.com/{repository}/{sha}/{name}'
     record['license_texts'].append(save(urllib.request.urlopen(raw,timeout=30).read(),raw))
  record['text_review_status']='license text obtained'if record['license_texts']else'license text missing'
  return record
 except Exception as e:record['error']=str(e);return record
with concurrent.futures.ThreadPoolExecutor(max_workers=5)as e:results=list(e.map(get,unique.values()))
(out/'rust-license-text-review.json').write_text(json.dumps(results,indent=2)+'\n')
print('Scanned',len(results),'packages; license text missing:',[(x['name'],x['version'])for x in results if not x['license_texts']]);print('Errors:',[(x['name'],x.get('error'))for x in results if x.get('error')])
