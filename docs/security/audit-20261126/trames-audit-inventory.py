from pathlib import Path
import sys
sys.path.insert(0,"/private/tmp/trames-license-audit-20261126/tools")
import tomli as tomllib
import json,subprocess,hashlib,collections,re
out=Path('/Users/main/dev/trames/docs/security/audit-20261126');out.mkdir(parents=True,exist_ok=True)
tmp=Path('/private/tmp/trames-license-audit-20261126')
repos={'ipad':Path('/Users/main/dev/trames'),'runtime':Path('/Users/main/dev/trames/runtime'),'mac':Path('/Users/main/dev/trames-mac'),'linux':Path('/Users/main/dev/trames-linux'),'cli':Path('/Users/main/dev/trames-cli')}
metadata={}
for n,r in repos.items():
 p=tmp/(n+'-metadata.json')
 if p.exists():
  try:metadata[n]=json.loads(p.read_text())
  except:pass
cache={}
for p in Path('/Users/main/.cargo/registry/src').glob('*/*/Cargo.toml'):cache[p.parent.name]=p
packages={};snapshots={};aux=[]
for n,r in repos.items():
 snapshots[n]={'path':str(r),'head':subprocess.check_output(['git','rev-parse','HEAD'],cwd=r,text=True).strip(),'status':subprocess.check_output(['git','status','--porcelain'],cwd=r,text=True).splitlines(),'submodules':subprocess.check_output(['git','submodule','status'],cwd=r,text=True).splitlines()}
 lock=tomllib.loads((r/'Cargo.lock').read_text())
 md={ (p['name'],p['version'],p.get('source')):p for p in metadata.get(n,{}).get('packages',[])}
 for item in lock['package']:
  key=(item['name'],item['version'],item.get('source'))
  p=md.get(key);mp=Path(p['manifest_path']) if p else cache.get(item['name']+'-'+item['version'])
  info={}
  if mp and mp.exists():info=tomllib.loads(mp.read_text()).get('package',{})
  # Missing source means first-party OR vendored patch: resolve metadata path.
  if not item.get('source') and (not p or '/vendor/' not in p['manifest_path']):continue
  rec=packages.setdefault(key,{'name':item['name'],'version':item['version'],'source':item.get('source') or 'vendored-path','checksum':item.get('checksum'),'license':p.get('license') if p else info.get('license'),'license_file':p.get('license_file') if p else info.get('license-file'),'repository':p.get('repository') if p else info.get('repository'),'homepage':p.get('homepage') if p else info.get('homepage'),'manifest':str(mp) if mp else None,'repositories':[],'evidence':[]})
  rec['repositories'].append(n)
  if mp and mp.exists() and not rec['evidence']:
   rec['manifest_sha256']=hashlib.sha256(mp.read_bytes()).hexdigest()
   for f in mp.parent.iterdir():
    if f.is_file() and re.match(r'(?i)^(licen[sc]e|copying|copyright|notice|unlicense)',f.name):
     rec['evidence'].append({'path':str(f),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()})
 for f in subprocess.check_output(['git','ls-files'],cwd=r,text=True).splitlines():
  if f.endswith('Cargo.lock') and f!='Cargo.lock':aux.append({'repo':n,'path':f,'packages':len(tomllib.loads((r/f).read_text()).get('package',[]))})
records=sorted(packages.values(),key=lambda x:(x['name'],x['version'],x['source']))
(out/'rust-dependencies.json').write_text(json.dumps(records,indent=2)+'\n')
(out/'snapshots.json').write_text(json.dumps(snapshots,indent=2)+'\n')
(out/'auxiliary-lockfiles.json').write_text(json.dumps(aux,indent=2)+'\n')
print('UNIQUE RUST',len(records),'license counts',dict(collections.Counter(str(x['license']) for x in records)))
print('PER REPO',{n:sum(n in x['repositories'] for x in records) for n in repos})
print('UNRESOLVED',[(x['name'],x['version']) for x in records if not x['license']])
print('NO TOP LICENSE',[(x['name'],x['version'],x['license']) for x in records if not x['evidence']])
print('NONPERMISSIVE',[(x['name'],x['version'],x['license']) for x in records if x['license'] and not re.search('MIT|Apache|BSD|Zlib|Unlicense|ISC',str(x['license']))])
print('AUX',aux)
