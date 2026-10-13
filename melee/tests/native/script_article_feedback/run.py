"""Actual owned Item actor/VM/pool Article feedback fixture, no game."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('feedback_source',HERE.parent/'script_form_feedback/extract.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);pieces=[];hashes={}
 for relative,names in [('src/sysdolphin/baselib/class.c',['ClassInfoInit','_hsdClassInit','hsdNew','hsdObjIsDescendantOf']),('src/sysdolphin/baselib/jobj.c',['JObjInit','HSD_JObjAlloc']),('src/melee/ef/eflib.c',['efLib_DestroyAll'])]:
  path=ROOT/relative;hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest();pieces += [extract.body(path.read_text(),name) for name in names]
 (out/'native_bodies.inc').write_text('\n\n'.join(pieces)+'\n')
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_binding_item_model_resolve','native_script_model_metadata','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_article_feedback','native_script_effect_dispatch']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_receiver_unavailable_identity.c',HERE/'test.c',HERE/'unselected_dependencies.c',HERE.parent/'script_item_actor/boundary.c']
 units += sorted(path for path in HERE.parent.glob('script_bindings_unavailable_*.c') if path.name not in ['script_bindings_unavailable_article_feedback.c','script_bindings_unavailable_presentation.c','script_bindings_unavailable_item.c','script_bindings_unavailable_item_birth.c'])
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c', ROOT/'src/melee/it/native_article_animation_origins.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+list(HERE.glob('*.h'))+[HERE/'run.py',HERE/'README.md',HERE.parent/'script_form_feedback/extract.py',a.compiler,out/'native_bodies.inc']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'));hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 command=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 commands=[]
 for stage,cmd in [('runtime',runtime_command),('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,'actual':['complete source compiler/catalog/VM/Item actor/storage','GObj creation/free/pool and source guards','JObj/class initialization/ancestry and actual effect destruction source bodies'],'controlled':['source entity immutable spec/provider and asset/model leaves','class allocator fixed test root pool','efSync particle-spawn leaf captures exact root or copied position plus optional scalar arguments; no renderer'],'limits':['no Item DAT constructor/game/particle appearance acceptance','JObj extent is engine class metadata; allocator has no per-JObj generation API','shared Attach_Scale creation currently has no script-calling creation hook; no arbitrary future hook guarantee']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
