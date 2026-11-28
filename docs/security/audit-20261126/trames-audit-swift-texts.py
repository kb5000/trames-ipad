from pathlib import Path
import json,urllib.request,concurrent.futures,tarfile,io,re,hashlib
r=Path('/Users/main/dev/trames');out=r/'docs/security/audit-20261126';cache=Path('/private/tmp/trames-license-audit-20261126/swift');cache.mkdir(exist_ok=True)
a=json.loads((out/'swift-dependencies.json').read_text());jobs=[]
for x in a:
 if x['name']in ['citadel','swift-nio-ssh']:
  u=json.loads((r/'vendor'/x['name']/'UPSTREAM.json').read_text());repo=u['repository'].split('github.com/',1)[1];rev=u['revision']
 else:
  repo=x['source'].split('github.com/',1)[1].removesuffix('.git');rev=x.get('revision')or ('v'+x['version'])
 jobs.append((x,repo,rev))
def get(job):
 x,repo,rev=job;p=cache/(x['name']+'-'+rev+'.tar.gz');rec={'name':x['name'],'repo':repo,'revision':rev,'license_texts':[]}
 try:
  url=f'https://codeload.github.com/{repo}/tar.gz/{rev}'
  if not p.exists():p.write_bytes(urllib.request.urlopen(url,timeout=90).read())
  rec['source_archive_sha256']=hashlib.sha256(p.read_bytes()).hexdigest();rec['source_url']=url
  with tarfile.open(p,'r:gz')as tf:
   for m in tf:
    if m.isfile()and re.search(r'(?i)(?:^|/)(LICENSE[^/]*|COPYING[^/]*|COPYRIGHT[^/]*|NOTICE[^/]*|UNLICENSE)$',m.name)and m.size<2000000:
     b=tf.extractfile(m).read();sha=hashlib.sha256(b).hexdigest();(out/'license-texts'/(sha+'.txt')).write_bytes(b);rec['license_texts'].append({'path':m.name,'sha256':sha,'text_file':'license-texts/'+sha+'.txt'})
  return rec
 except Exception as e:rec['error']=str(e);return rec
with concurrent.futures.ThreadPoolExecutor(max_workers=4)as e:results=list(e.map(get,jobs))
(out/'swift-license-text-review.json').write_text(json.dumps(results,indent=2)+'\n')
print('Swift source/license archives',len(results),'missing',[(x['name'],x.get('error'))for x in results if not x['license_texts']])
