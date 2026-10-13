"""Hidden actual Item actor/source VM/accessory scheduler console fixture."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];SCHEDULER=HERE.parent/'script_item_scheduler'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True)
 spec=importlib.util.spec_from_file_location('scheduler_extract',SCHEDULER/'extract_scheduler.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract);hashes=extract.extract(ROOT,out/'actual_scheduler.h')
 boundary=(SCHEDULER/'boundary.c').read_text();absent='READ(nativeScriptBindingItem_Read) WRITE(nativeScriptBindingItem_Write) CALL(nativeScriptBindingItem_Invoke)'
 if boundary.count(absent)!=1:raise RuntimeError('controlled Item boundary changed')
 boundary=boundary.replace(absent,'').replace('#include "boundary.h"','#include "boundary.h"\nunsigned accessory_tail_calls;')
 for name in ['it_80271A58','it_80272280','it_802722B0']:
  if boundary.count('LEAF('+name+')')!=1:raise RuntimeError('accessory tail boundary changed')
  boundary=boundary.replace('LEAF('+name+')','void '+name+'(HSD_GObj*);')
 (out/'boundary.c').write_text(boundary+'\n'+(HERE/'unselected.c').read_text(),newline='\n')
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_effect_resolve','binding_spawn_resolve','binding_item_model_resolve','model_metadata','binding_article_reference_resolve','binding_signal_resolve','compile', 'binding_item_resources_resolve','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','binding_item','item_program', 'resource_metadata','metadata','pose_metadata']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]
 units += [ROOT/'src/melee/ft'/(n+'.c') for n in ['native_callback_lifetime','native_fighter_pose_path','native_source_declarations','native_source_asset_validation','ftcharacter_asset_layout','native_fighter_sound_policy','native_source_motion_policy']]
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot']]
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 # Other native binding families reject. Their absence is never an accepted
 # authored callback, metadata validator or actor execution boundary.
 units += sorted(f for f in HERE.parent.glob('script_bindings_unavailable_*.c') if f.name not in ['script_bindings_unavailable_item_birth.c','script_bindings_unavailable_item.c','script_bindings_unavailable_kinetics.c','script_bindings_unavailable_state.c','script_bindings_unavailable_combat.c','script_bindings_unavailable_setup.c','script_bindings_unavailable_presentation.c','script_bindings_unavailable_transition.c'])
 units += [HERE.parent/'script_effect_actor_absent.c',HERE/'test.c',out/'boundary.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=units+list(HERE.glob('*'))+[SCHEDULER/'boundary.c',SCHEDULER/'boundary.h',SCHEDULER/'extract_scheduler.py',HERE.parent/'script_kinetics/extract_primitives.py',ROOT/'docs/scripting/item.md',a.compiler.resolve()]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(f.resolve()):sha(f) for f in inputs if f.is_file()})
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(SCHEDULER)]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime);commands=[]
 compile_runtime=[str(a.compiler),*common,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 compile=[str(a.compiler),*common,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 for stage,cmd in [('runtime',compile_runtime),('compile',compile),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-12000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'commands':commands,'input_sha256':hashes,'actual':['source frontend, real immutable Item program preparation and actor program validation','bytecode/VM/helper/signal execution, actual Item actor code-ID callback registration','actual Item_80269A9C accessory tick, hitlag suppression and state reset/teardown bodies','real GObj fixed pool, Item storage and packed actor snapshot roundtrip'],'controlled':['owned Article/common-attribute/model descriptor fixture data and prepared-program publication lookup','native effect queue, graphics, physics, audio and scheduler-tail leaves','PositionX write used for synchronous native-removal and VM-fault injection; unrelated native self operations abort'],'limits':['no DAT animation import, full constructor/scene/render or game acceptance','only declared accessory registration and explicit clear; other callback registration families unchanged']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
