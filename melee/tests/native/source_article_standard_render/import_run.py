import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--link',type=Path,required=True);p.add_argument('--young-link',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);build=ROOT/'build/native';ninja=(build/'build.ninja').read_text();start=ninja.index('build native_source_assets_test.exe:');block=ninja[start:ninja.index('\n\n',start)];libs=re.search(r'^  LINK_LIBRARIES = (.+)$',block,re.M).group(1).split()
 units=[HERE/'import.c',ROOT/'src/melee/it/native_item_model_admission.c',ROOT/'src/melee/it/itcommand_data.c']
 units += [ROOT/'src/melee/ft'/(n+'.c') for n in ['native_script_item_program','native_script_binding_item_model_resolve','native_script_model_metadata','native_source_motion_policy','native_source_entity_assets','native_source_bank','native_source_asset_import','native_source_asset_validation','ftcharacter_asset_layout','native_source_declarations']]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c']
 paths=[build/x for x in libs if x.endswith('.lib') and not x.startswith('-')];inputs=units+[HERE/'import_run.py',HERE/'README.md',a.compiler,build/'build.ninja',*paths,*list((ROOT/'src').rglob('*.h'))]
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();hashes={str(path):sha(path) for path in inputs if path.exists()};assets={str(p):sha(p) for d in [a.link,a.young_link] for p in [d/'assets.fma',d/'fighter.dat',d/'animation.dat',d/'costume.dat']}
 command=[str(a.compiler),'-std=gnu11','-O2','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt','-fuse-ld=lld','-Wl,/opt:ref','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,units),*libs,'-o',str(out/'test.exe')];commands=[]
 for dll in build.glob('*.dll'):shutil.copy2(dll,out/dll.name)
 for stage,cmd,cwd in [('compile',command,build),('run',[str(out/'test.exe'),str(a.link),str(a.young_link)],out)]:
  r=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-10000:])
 for f,h in {**hashes,**assets}.items():
  if sha(Path(f))!=h:raise RuntimeError('input changed '+f)
 receipt={'status':'PASS','result':[json.loads(x) for x in r.stdout.splitlines() if x.startswith('{')],'commands':commands,'source_sha256':hashes,'owned_dat_sha256':assets,'scope':'actual copied Link/YoungLink DAT/facts imports, source metadata/compiler/program preparation, native HSD graph construction and shared instance-aware topology admission; no draw GPU/game/complete Link acceptance'}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
