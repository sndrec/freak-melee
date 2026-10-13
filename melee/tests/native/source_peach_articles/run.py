"""Private real-library Peach primary-Article constructor cohort; never builds shared targets."""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--iso',type=Path,required=True);p.add_argument('--package',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
 meta=json.loads(a.package.read_text());build=ROOT/'build/native';ninja=(build/'build.ninja').read_text();s=ninja.index('build native_source_assets_test.exe:');block=ninja[s:ninja.index('\n\n',s)]
 libs=re.search(r'^  LINK_LIBRARIES = (.+)$',block,re.M).group(1).split()
 inputs=[a.compiler.resolve(),a.package.resolve(),HERE/'test.c',HERE/'run.py',HERE/'README.md',build/'build.ninja']+[build/x for x in libs if x.endswith('.lib') and not x.startswith('-')]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'src').rglob('*.c'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+list(build.glob('*.dll'))
 cache=Path(meta['cache']);inputs+=list(x for x in cache.rglob('*') if x.is_file())
 sha=lambda x:hashlib.sha256(x.read_bytes()).hexdigest();hashes={str(x.resolve()):sha(x) for x in inputs if x.exists()}
 out.mkdir(parents=True);shutil.copytree(cache,out/'cache');shutil.copy2(a.package,out/'package-input.json')
 cmd=[str(a.compiler.resolve()),'-std=gnu11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt','-fuse-ld=lld','-Wl,/opt:ref','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(ROOT.parent/'aurora/include'),str(HERE/'test.c')]+libs+['-o',str(out/'test.exe')]
 commands=[];results=[]
 for name,command,cwd in [('compile',cmd,build),('test',[str(out/'test.exe'),str(a.iso.resolve()),str(out/'cache'),meta['id']],out)]:
  if name=='test':
   for dll in build.glob('*.dll'):shutil.copy2(dll,out/dll.name)
  r=subprocess.run(command,cwd=cwd,capture_output=True,text=True,timeout=90,creationflags=subprocess.CREATE_NO_WINDOW);(out/(name+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':name,'command':command,'exit':r.returncode})
  results += [json.loads(x) for x in r.stdout.splitlines() if x.startswith('{')]
  (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:break
 unchanged=all(sha(Path(x))==v for x,v in hashes.items());receipt={'status':'PASS' if r.returncode==0 and unchanged else 'FAIL','commands':commands,'results':results,'input_sha256':hashes,'inputs_unchanged':unchanged,'scope':'Real copied Peach packet; Runtime_Load performs source preparation, activation lease and publication. Real HSD heap/GObj/Item/Actor/VM/animation/DATA and all-five native removal; no substituted production functions. Minimal parent Fighter payload initialization and storage handle defaults are explicit fixture setup.', 'limits':['Not full Fighter_Init, graphics, audio, complete stage/tick scheduler or collision acceptance.','Explode/Spore effect-producing Created callbacks are not invoked. Turnip Created and all13 authored Animation phases run.','Item packed snapshot roundtrip changes only actor-owned creation_value; native model/Item peers stay live. Not whole-match rollback.','Explicit VM count excludes constructor absent Spawned slots and native-removal Destroyed/nested recipient callbacks.','The SDK duplicate MTXPerspective array bounds warning is suppressed; all other fixture warnings are errors.']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'status':receipt['status'],'results':results,'receipt':str(out/'receipt.json')}));print((r.stdout+r.stderr)[-7000:]);raise SystemExit(r.returncode or (not unchanged))
if __name__=='__main__':main()
