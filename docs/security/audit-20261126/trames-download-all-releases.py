from pathlib import Path
import json,urllib.request,concurrent.futures,time,hashlib,os
root=Path('/private/tmp/trames-license-audit-20261126/releases');root.mkdir(exist_ok=True)
out=Path('/Users/main/dev/trames/docs/security/audit-20261126');data=json.loads((out/'published-releases.json').read_text());jobs=[]
for repo,rels in data.items():
 for rel in rels:
  for a in rel['assets']:jobs.append((repo,rel['tag_name'],a))
def get(job):
 repo,tag,a=job;p=root/repo/tag/a['name'];p.parent.mkdir(parents=True,exist_ok=True)
 for attempt in range(3):
  try:
   if not p.exists()or p.stat().st_size!=a['size']:
    req=urllib.request.Request(a['browser_download_url'],headers={'User-Agent':'Trames-license-audit/1.0'})
    with urllib.request.urlopen(req,timeout=90)as src,p.with_suffix(p.suffix+'.part').open('wb')as dst:
     while True:
      b=src.read(1024*1024)
      if not b:break
      dst.write(b)
    p.with_suffix(p.suffix+'.part').replace(p)
   assert p.stat().st_size==a['size'], 'Size mismatch'
   sha=hashlib.sha256(p.read_bytes()).hexdigest();expected=a.get('digest')
   if expected and expected.startswith('sha256:'):assert sha==expected.split(':',1)[1],'GitHub digest mismatch'
   return {'repository':repo,'tag':tag,'name':a['name'],'path':str(p),'bytes':p.stat().st_size,'sha256':sha,'github_digest':expected,'url':a['browser_download_url']}
  except Exception as e:
   if attempt==2:return {'repository':repo,'tag':tag,'name':a['name'],'error':str(e)}
   time.sleep(1)
records=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=5)as pool:
 for i,rec in enumerate(pool.map(get,jobs),1):
  records.append(rec)
  if i%10==0 or 'error'in rec:print('Downloaded',i,'/',len(jobs),flush=True)
(out/'downloaded-release-assets.json').write_text(json.dumps(records,indent=2)+'\n')
print('COMPLETE',len(records),'errors',[x for x in records if 'error'in x],flush=True)
