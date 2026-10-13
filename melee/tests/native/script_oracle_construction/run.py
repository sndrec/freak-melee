"""Bounded actual C-oracle caller migration checks; no game/global build."""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);units=[ROOT/u for u in json.loads((HERE/'units.json').read_text())]
 flags=['-std=gnu11','-O2','-ffunction-sections','-fdata-sections','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(ROOT/'tools')]
 commands=[]
 def run(stage,cmd):
  r=subprocess.run(cmd,cwd=out,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  return r
 r=run('tools',[str(a.compiler),*flags,'-c',*map(str,units)])
 if r.returncode:raise RuntimeError(r.stderr[-5000:])
 objects=list(out.glob('*.o'))+list(out.glob('*.obj'));results=[]
 files=[ROOT/'tests/native'/f for f in ['article_animation_components_compiler_test.c','article_helpers_compiler_test.c','script_typed_locals_compiler_test.c','script_state_compiler_test.c','script_callback_exports_compiler_test.c','script_callback_lifetime_compiler_test.c','script_engine_items_compiler_test.c','script_item_pause_compiler_test.c','script_model_effect_compiler_test.c','ness_magnet_primitives_test.c','script_cpu_projectile_defense_compiler_test.c','script_held_motion_compiler_test.c']]
 for n,source in enumerate(files):
  exe=out/(source.stem+'.exe');r=run('build'+str(n),[str(a.compiler),*flags,'-fuse-ld=lld','-Wl,/opt:ref',str(source),*map(str,objects),'-o',str(exe)])
  if r.returncode:raise RuntimeError(r.stderr[-5000:])
  r=run('test'+str(n),[str(exe)]);results.append({'file':str(source),'exit':r.returncode,'output':r.stdout+r.stderr})
 (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
 for r in results:
  if r['exit']:print(r['file']+' '+r['output'][-2000:])
 print('samplers',len(results),'failures',sum(r['exit']!=0 for r in results))
 if any(r['exit'] for r in results):raise SystemExit(1)
 # The real test-only CLI has an explicit construction tuple and preserves output
 # on malformed requests; no source base inference is available.
 cli=out/'reference.exe';r=run('cli-build',[str(a.compiler),*flags,'-fuse-ld=lld','-Wl,/opt:ref',str(ROOT/'tests/native/script_reference_exporter.c'),*map(str,objects),'-o',str(cli)])
 if r.returncode:raise RuntimeError(r.stderr[-5000:])
 source=out/'behavior.fms';source.write_text('move Owned:\n    on state_341_physics:\n        pass\n')
 target=out/'module.c'
 command=[str(cli),str(source),'explicit_oracle',str(target),'--construction','ftDataTest','341','1']
 r=run('cli-positive',command)
 if r.returncode:raise RuntimeError(r.stderr)
 code=target.read_text();assert 'strcmp(base->fighter_root, "ftDataTest")' in code and 'base->motion_state_count != 1' in code
 accepted=target.read_bytes()
 bad=[command[:-1]+['0'],command[:-2]+['65536','1'],command[:-3]+['bad root','341','1'],command[:4]]
 for i,cmd in enumerate(bad):
  r=run('cli-bad'+str(i),cmd);assert r.returncode and target.read_bytes()==accepted
 # Real action containers use the same explicit table contract; no named move
 # contextual aliases are introduced by grouping multiple entry callbacks.
 group=out/'ordinary-action.fms';group.write_text('action Lifecycle:\n    on load:\n        pass\n    on load_special_attrs:\n        pass\n')
 r=run('ordinary-action',[str(cli),str(group),'lifecycle',str(out/'lifecycle.c'),'--construction','ftDataTest','341','1'])
 if r.returncode:raise RuntimeError(r.stderr)
 group=out/'climbers.fms';group.write_text('move IceShot:\n    on special_n:\n        pass\n    on special_air_n:\n        pass\nmove Blizzard:\n    on special_lw:\n        pass\n    on special_air_lw:\n        pass\n')
 for member in ['Popo','Nana']:
  target=out/(member+'.c');r=run('moves-'+member,[str(cli),str(group),'climbers',str(target),'--construction','ftData'+member,'341','26'])
  if r.returncode:raise RuntimeError(r.stderr)
  text=target.read_text();assert all('generated.'+field+' = authored_' in text for field in ['special_n','special_air_n','special_lw','special_air_lw'])
 group.write_text('action Outside:\n    on state_342_physics:\n        pass\n')
 r=run('action-bounds',[str(cli),str(group),'bounded',str(out/'outside.c'),'--construction','ftDataTest','341','1']);assert r.returncode
 receipt={'status':'PASS','samplers':len(results),'cli_cases':9,'boundaries':['actual test-only C generator and migrated mixed-mode caller samplers','actual exporter explicit tuple validation/output preservation'],'limits':['not direct VM or game acceptance; this stage only preserves behavioral C oracle construction'],'commands':commands,'results':results}
 paths=units+files+[ROOT/'tools/native_fighter_script.h',ROOT/'tests/native/script_reference_exporter.c',HERE/'run.py',HERE/'units.json']
 receipt['source_sha256']={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('PASS oracle construction')
if __name__=='__main__':main()
