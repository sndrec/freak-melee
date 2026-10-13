"""Private actual-library scene owner proof; no shared build/game."""
import argparse, hashlib, json, re, shutil, subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--link',type=Path,required=True);p.add_argument('--fox',type=Path);p.add_argument('--falco',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 build=ROOT/'build/native';ninja=(build/'build.ninja').read_text();begin=ninja.index('build native_source_assets_test.exe:');block=ninja[begin:ninja.index('\n\n',begin)];libs=re.search(r'^  LINK_LIBRARIES = (.+)$',block,re.M).group(1).split()
 inputs=[a.compiler.resolve(),build/'build.ninja',HERE/'test.c',HERE/'run.py',HERE/'README.md']
 inputs += list(HERE.glob('*.h'))
 inputs += [build/n for n in libs if n.endswith('.lib') and not n.startswith('-')]
 inputs += list((ROOT/'src').rglob('*.c'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+list(build.glob('*.dll'))
 for project in [a.link,a.fox,a.falco]:
  if project:inputs += [project/n for n in ['project.txt','assets.fma','fighter.dat','animation.dat','costume.dat','cpu_profile.fcp']]
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();hashes={str(x.resolve()):sha(x) for x in inputs if x.exists()}
 out.mkdir(parents=True);shutil.copytree(a.link,out/'draft')
 for dll in build.glob('*.dll'):shutil.copy2(dll,out/dll.name)
 cmd=[str(a.compiler.resolve()),'-std=gnu11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt','-fuse-ld=lld','-Wl,/opt:ref','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(ROOT.parent/'aurora/include'),str(HERE/'test.c'),str(ROOT/'src/melee/it/native_article_model_scene.c')]+libs+['-o',str(out/'test.exe')]
 commands=[];results=[]
 runs=[('compile',cmd,build),('fixture',[str(out/'test.exe'),str(out/'draft')],out)]
 for form in ['fox','falco']:
  project=getattr(a,form)
  if project:
   shutil.copytree(project,out/form)
   declaration='It_Kind_Fox_Illusion' if form=='fox' else 'It_Kind_Falco_Phantasm'
   runs.append(('fixture-'+form,[str(out/'test.exe'),str(out/form),declaration],out))
 for stage,command,cwd in runs:
  r=subprocess.run(command,cwd=cwd,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':command,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  results += [json.loads(line) for line in r.stdout.splitlines() if line.startswith('{')]
  if r.returncode:break
 unchanged=all(sha(Path(path))==value for path,value in hashes.items());status='PASS' if r.returncode==0 and unchanged else 'FAIL'
 receipt={'status':status,'results':results,'commands':commands,'input_sha256':hashes,'inputs_unchanged':unchanged,'scope':'actual staged source candidate, compiler/program/Item actor/VM/GObj/package pins and scene/core/heap/graph/matrix lifetime','controlled':['private runtime and main-heap transition tables receive explicit inputs; no function substitution','secondary root table imported through source metadata from actual owned Link DAT before publication','Item userdata initialization/cleanup; real actor attach and VM animation phase'],'limits':['no live renderer/physics/audio/scheduler scene launch','no Workshop/package-loader gameplay admission','no initialized page history or whole-match rollback proof']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'status':status,'results':results,'receipt':str(out/'receipt.json')}));print((r.stdout+r.stderr)[-5000:]);raise SystemExit(r.returncode or (not unchanged))
if __name__=='__main__':main()
