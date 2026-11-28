from pathlib import Path
import json,subprocess,re,tarfile,hashlib,collections,sys
sys.path.insert(0,'/private/tmp/trames-license-audit-20261126/tools');import tomli
r=Path('/Users/main/dev/trames');out=r/'docs/security/audit-20261126'
repos={'ipad':r,'runtime':r/'runtime','mac':Path('/Users/main/dev/trames-mac'),'linux':Path('/Users/main/dev/trames-linux'),'cli':Path('/Users/main/dev/trames-cli')}
urls=[];actions=[];notices=[];binary=[]
for name,root in repos.items():
 for rel in subprocess.check_output(['git','ls-files'],cwd=root,text=True).splitlines():
  p=root/rel
  if not p.is_file() or p.stat().st_size>1000000:continue
  if not (rel.startswith(('scripts/','.github/workflows/','crates/')) or p.name in ['Cargo.toml','Package.swift']):continue
  if p.suffix not in ['.py','.sh','.yml','.yaml','.rs','.c','.h','.toml','.json']:continue
  try:s=p.read_text()
  except:continue
  if rel.startswith('.github/workflows/'):
   for line,txt in enumerate(s.splitlines(),1):
    for action in re.findall(r'uses:\s*[\'\"]?([^\s\'\"]+)',txt):
     if not action.startswith(('./','docker://')):actions.append({'repo':name,'path':rel,'line':line,'action':action})
  if rel.startswith('scripts/'):
   for line,txt in enumerate(s.splitlines(),1):
    for url in re.findall(r'https?://[^\s\'\"<>]+',txt):urls.append({'repo':name,'path':rel,'line':line,'url_expression':url[:400]})
(out/'build-input-urls.json').write_text(json.dumps(urls,indent=2)+'\n')
(out/'github-actions.json').write_text(json.dumps(actions,indent=2)+'\n')
# All locally staged Alpine packages, independent of whether production strips test assets.
apks={}
for base in [r/'apps/ios/Trames/GuestVulkan',r/'target/vulkan-guest']:
 if not base.exists():continue
 for f in base.rglob('*.apk'):
  try:
   with tarfile.open(f,'r:gz',ignore_zeros=True) as tf:
    member=next((m for m in tf if m.name in ['.PKGINFO','./.PKGINFO']),None)
    if not member:continue
    fields={}
    for line in tf.extractfile(member).read().decode(errors='replace').splitlines():
     if ' = ' in line:
      k,v=line.split(' = ',1);fields.setdefault(k,[]).append(v)
    key=(fields.get('pkgname',['?'])[0],fields.get('pkgver',['?'])[0])
    rec=apks.setdefault(key,{'name':key[0],'version':key[1],'license':fields.get('license',[]),'origin':fields.get('origin',[]),'url':fields.get('url',[]),'dependencies':fields.get('depend',[]),'paths':[]})
    rec['paths'].append(str(f))
  except Exception as e:print('APK ERROR',str(f),str(e))
(out/'staged-apk-packages.json').write_text(json.dumps(sorted(apks.values(),key=lambda x:x['name']),indent=2)+'\n')
# Swift lock + local forks + independently selected Xcode package.
pins=json.loads((r/'apps/ios/TramesKit/Package.resolved').read_text())['pins'];swift=[]
for x in pins:
 key=x['identity'];matches=list((r/'apps/ios/Trames/Notices').glob(key+'-*'))
 swift.append({'name':key,'version':x['state'].get('version'),'revision':x['state']['revision'],'source':x['location'],'notice_files':[str(p)for p in matches]})
for name in ['citadel','swift-nio-ssh']:
 p=r/'vendor'/name
 swift.append({'name':name,'source_type':'vendored fork','provenance':str(p/'UPSTREAM.json'),'license_files':[str(f)for f in p.iterdir() if f.name.lower().startswith('license')]})
swift.append({'name':'SwiftTerm','version':'1.20.0','source':'https://github.com/migueldeicaza/SwiftTerm','pin_evidence':'apps/ios/project.yml','license_files':[str(r/'apps/ios/Trames/Notices/SwiftTerm-LICENSE.txt')]})
(out/'swift-dependencies.json').write_text(json.dumps(swift,indent=2)+'\n')
# Exact pinned native and Android source inputs.
for n in ['vulkan-upstream','android-upstream']:
 (out/(n+'.json')).write_bytes((r/'scripts'/(n+'.json')).read_bytes())
for base in [r/'apps/ios/Trames/Notices',r/'apps/ios/Trames/GuestVideo/Notices',r/'apps/ios/Trames/GuestVulkan/Notices',r/'runtime/crates/trames-native/upstream']:
 if base.exists():
  for p in base.rglob('*'):
   if p.is_file() and (re.search('(?i)license|copying|notice|copyright',p.name)):
    notices.append({'path':str(p),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(out/'notice-evidence.json').write_text(json.dumps(notices,indent=2)+'\n')
print('Unique action refs:',sorted(set(x['action']for x in actions)))
print('Build URL expressions',len(urls),'Swift records',len(swift),'staged APKs',len(apks),'notice evidence',len(notices))
print('APK licenses',dict(collections.Counter(';'.join(x['license']) for x in apks.values())))
