"""Actual copied costume import and raw-part ancestor census; no live graph."""
import hashlib,json,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[3]
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=False)
compiler=Path('C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe')
units=[root/'tests/native/script_target_bone_queries/costume_probe.c',root/'src/melee/ft/ftcostume_assets.c',
       root/'src/melee/it/native_article_animation_texture.c']
units += [root/'src/sysdolphin/baselib'/(n+'.c') for n in ['dat','dat_import','dat_geometry','dat_material','dat_model','dat_animation','dat_scene','dat_texture']]
cmd=[str(compiler),'-std=gnu11','-O2','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(root/'src'),'-I'+str(root/'extern/dolphin/include'),*map(str,units),'-o',str(out/'probe.exe')]
r=subprocess.run(cmd,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW);(out/'compile.log').write_text(r.stdout+r.stderr)
if r.returncode:raise RuntimeError(r.stderr[-12000:])
result=[];inputs=units+[Path(__file__),compiler]
for i in [8,9]:
 p=Path(f'A:/FreakMelee-test-archives/fighter-callback-source-census/fighter-{i}/costume.dat');inputs +=[p,p.with_name('project.txt'),p.with_name('assets.fma')]
 r=subprocess.run([str(out/'probe.exe'),str(p)],capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
 if r.returncode:raise RuntimeError(str(r.returncode)+': '+r.stderr)
 result.append({'costume':str(p),'census':json.loads(r.stdout)})
receipt={'status':'PASS','results':result,'inputs':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},'scope':'actual typed costume DAT importer and raw preorder descriptor ancestor census; no runtime construction or matrix evaluation'}
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(result))
