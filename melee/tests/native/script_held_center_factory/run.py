"""Held-center source factory and actual constructor/actor/VM cohort."""
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
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['effect','signal','feedback','jump','forms','ledge','actions', 'item_resources','article_reference','article_lifecycle','vectors', 'hitlag', 'stage', 'family']]
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
 units += [HERE/'test.c',out/'boundary.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'tests/native/script_bindings_unavailable_item_animation.c']
 for leaf in sorted((ROOT/'tests/native').glob('script_bindings_unavailable_*.c')):
  if leaf not in units and leaf.stem.replace('script_bindings_unavailable_','') not in {'kinetics','state','combat','presentation','transition','item','setup','spawn','item_birth'}:units.append(leaf)
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',HERE/'extract.py',HERE/'README.md',ROOT/'docs/scripting/held_center_factory.md',compiler]+list(HERE.glob('*.h'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs});commands=[]
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(HERE),'-I'+str(ROOT/'tests/native/script_item_scheduler'),'-I'+str(ROOT/'tests/native/script_item_spawn')]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 runtime_command=[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 command=[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref']+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 strict_command=[str(compiler)]+common+['-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-sign-compare','-fsyntax-only',str(HERE/'test.c'),str(ROOT/'src/melee/it/native_script_item_spawn.c'),str(ROOT/'src/melee/ft/native_script_binding_spawn.c'),str(ROOT/'src/melee/ft/native_script_binding_spawn_resolve.c')]
 for stage,cmd in [('strict-owned',strict_command),('compile-runtime',runtime_command),('compile',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-9000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'boundaries':['actual Item_CreateAuthored/internal constructor, Item_8026AB54 pickup and state/removal bodies','actual source frontend/catalog/op9/VM/Fighter and Item actors/storage/receivers/fixed GObj pool; real SourceAlive/SourceFighterLayout and creator capture','controlled assets/model/platform/audio leaves; stage failure injection instrumentation'],'limits':['no game launch/full scene or archive/render parity','selected package/facts/program/spec and model/animation/native attachment leaves controlled; no DAT loader/publisher/render/game acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':receipt['summary']}))
if __name__=='__main__':main()
