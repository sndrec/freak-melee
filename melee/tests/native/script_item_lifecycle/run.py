"""Hidden actual Item actor/source VM/lifecycle scheduler console fixture."""
import argparse,hashlib,importlib.util,json,subprocess,sys,re
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
 boundary=boundary.replace(absent,'').replace('#include "boundary.h"','#include "boundary.h"\n#include <melee/it/native_item_lifetime.h>\n#include <melee/ft/native_script_binding_item_lifecycle.h>')
 helper='\n'.join(extract.reference.body(re.sub(r'(void it_8026BDB4\(HSD_GObj\* gobj\))[^\n]*',r'\1',extract.native_only((ROOT/path).read_text())),name) for path,names in [('src/melee/it/it_2725.c',['it_802750F8','it_8027518C','it_802751D8']),('src/melee/it/it_26B1.c',['it_8026BDB4','it_8026B3A8'])] for name in names)
 (out/'actual_scheduler.h').write_text((out/'actual_scheduler.h').read_text()+'\n'+helper,newline='\n')
 hashes[str((ROOT/'src/melee/it/it_2725.c').resolve())]=hashlib.sha256((ROOT/'src/melee/it/it_2725.c').read_bytes()).hexdigest()
 # Fault/model/schema injections are observable callback leaves, never fake VM success.
 write='item->pos.x=v->as.f32;'
 assert boundary.count(write)==1
 boundary=boundary.replace(write,write+'if(!lifecycle_observe(c,v))return false;')
 boundary=boundary.replace('#include "boundary.h"','#include "boundary.h"\nbool lifecycle_observe(NativeScriptRuntimeContext*,const NativeScriptValue*);',1)
 (out/'boundary.c').write_text(boundary+'\n'+(HERE/'unselected.c').read_text(),newline='\n')
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_effect_resolve','binding_spawn_resolve','binding_item_model_resolve','model_metadata','binding_article_reference_resolve','binding_signal_resolve','compile', 'binding_item_resources_resolve','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','binding_item','binding_item_lifecycle','item_program', 'resource_metadata','metadata','pose_metadata']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]
 units += [ROOT/'src/melee/ft'/(n+'.c') for n in ['native_callback_lifetime','native_fighter_pose_path','native_source_declarations','native_source_asset_validation','ftcharacter_asset_layout','native_fighter_sound_policy','native_source_motion_policy']]
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot']]
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 # Other native binding families reject. Their absence is never an accepted
 # authored callback, metadata validator or actor execution boundary.
 units += sorted(f for f in HERE.parent.glob('script_bindings_unavailable_*.c') if f.name not in ['script_bindings_unavailable_item_birth.c','script_bindings_unavailable_item_lifecycle.c','script_bindings_unavailable_item.c','script_bindings_unavailable_kinetics.c','script_bindings_unavailable_state.c','script_bindings_unavailable_combat.c','script_bindings_unavailable_setup.c','script_bindings_unavailable_presentation.c','script_bindings_unavailable_transition.c'])
 units += [HERE.parent/'script_effect_actor_absent.c',HERE/'test.c',out/'boundary.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=units+list(HERE.glob('*'))+[SCHEDULER/'boundary.c',SCHEDULER/'boundary.h',SCHEDULER/'extract_scheduler.py',HERE.parent/'script_kinetics/extract_primitives.py',ROOT/'src/melee/it/it_2725.c',ROOT/'docs/scripting/item.md',ROOT/'docs/scripting/presentation.md',ROOT/'docs/scripting/article_helpers.md',a.compiler.resolve()]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(f.resolve()):sha(f) for f in inputs if f.is_file()})
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(SCHEDULER)]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime);commands=[]
 compile_runtime=[str(a.compiler),*common,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 compile=[str(a.compiler),*common,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 for stage,cmd in [('runtime',compile_runtime),('compile',compile),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-12000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'commands':commands,'input_sha256':hashes,'actual':['source frontend, immutable Item program preparation, actor bytecode/VM signals and phases','real it_802750F8 and Item physics/collision/removal scheduler bodies','actual native it_8027518C/751D8, it_8026B3A8/BDB4 complete Item-byte comparisons across10 timer values','fixed GObj/proc pools, deferred native deletion and actual Item owned storage retirement','packed actor component save/load with explicit native Item peer restoration'],'controlled':['Article/attributes/model fixture data and accepted program lookup','geometry, animation, graphics/audio and effect destruction leaves','PositionX callback leaf used to inject logical faults, native removal and peer identity mutation'],'limits':['no full Item DAT/assets/render/game acceptance','component snapshot requires matching native Item/GObj/model/program peers']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
