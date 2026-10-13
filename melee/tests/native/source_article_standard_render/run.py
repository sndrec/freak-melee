"""Connected source factory, actual constructor/scheduler/actors/VM fixture."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('spawn_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 hashes=extractor.extract(ROOT,out);compiler=a.compiler.resolve()/'clang.exe'
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_spawn','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_callback_lifetime','native_script_actor_access','native_script_receiver','native_script_binding_receiver','native_script_fighter_actor']
 units=[ROOT/'src/melee/ft'/(n+'.c') for n in names]
 units += [ROOT/'src/melee/ft'/(n+'.c') for n in ['native_script_binding_effect_resolve','native_script_binding_signal_resolve','native_script_binding_article_reference_resolve']]
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['effect','signal','feedback','jump','forms','ledge','actions','article_reference','article_lifecycle','vectors']]
 units += [ROOT/'tests/native/script_effect_actor_absent.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/ft/native_script_binding_spawn_resolve.c']
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['cpu_control','targets','item_model','article_combat','world_feedback','article_motion']]
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot','native_script_item_spawn']]
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += [HERE/'construction.c',out/'boundary.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'src/melee/ft'/(n+'.c') for n in ['native_source_motion_policy','native_script_resource_metadata','native_script_item_program', 'native_script_binding_item_animation_prepare','native_script_metadata','native_source_declarations','native_source_asset_validation','ftcharacter_asset_layout','native_fighter_sound_policy']]
 units += [ROOT/'src/sysdolphin/baselib/dat.c']
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c']
 units += sorted(p for p in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if p.name not in ['script_bindings_unavailable_spawn.c','script_bindings_unavailable_actions.c','script_bindings_unavailable_common_properties.c','script_bindings_unavailable_item.c','script_bindings_unavailable_item_birth.c','script_bindings_unavailable_presentation.c'])
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',HERE/'extract.py',HERE/'README.md',compiler]+list(HERE.glob('*.h'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 units=list(dict.fromkeys(units));hashes.update({str(path):sha(path) for path in inputs});commands=[]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c']
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(HERE),'-I'+str(ROOT/'tests/native/script_item_spawn'),'-I'+str(ROOT/'tests/native/script_item_scheduler')]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 runtime_command=[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 command=[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref']+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 for stage,cmd in [('compile-runtime',runtime_command),('compile',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-9000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'boundaries':['actual typed Source compiler and Item metadata/program preparation','actual source Item behavior selector, Item actor/VM/execution/GObj constructor and standard draw body','controlled HSD load/display, physical item allocation and unrelated native physics/platform/audio leaves'],'limits':['program lookup publication is controlled; no real runtime source package selection','synthetic primary model in constructor fixture; retail import/native HSD graph proof is separate','no secondary models, outline pass, billboard GPU, game or complete Link activation']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':receipt['summary']}))
if __name__=='__main__':main()
