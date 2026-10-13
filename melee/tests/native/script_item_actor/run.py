"""Actual Item actor/VM/GObj lifetime fixture; no client launch."""
import argparse,hashlib,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);compiler=a.compiler.resolve()/'clang.exe'
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_script_binding_item_model_resolve','native_script_model_metadata','native_script_hitbox_metadata','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve']]
 units += [HERE.parent/('script_bindings_unavailable_'+name+'.c') for name in ['world_feedback','item_model','article_motion','article_combat','targets','cpu_control','article_lifecycle','vectors','signal','feedback','jump','forms','ledge','article_reference', 'hitlag', 'stage', 'family']]
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state','native_match_arena','native_match_scene']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_bindings_unavailable_actions.c',HERE.parent/'script_bindings_unavailable_effect.c',HERE.parent/'script_bindings_unavailable_spawn.c',HERE/'test.c',HERE/'boundary.c',HERE.parent/'script_receiver_unavailable_identity.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'tests/native/script_bindings_unavailable_item_animation.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['item_lifecycle','item_collision','item_observations','item_terrain','audio','ground_controls','model_graph','hitbox_create','fighter_hitboxes','presentation','attachments','guard','air_inputs','contact','fighter_observations','air_physics','absorber','capture','collision','held_state','held_items']]
 units += [ROOT/'src/melee/gr'/(name+'.c') for name in ['native_ground_pool','native_green_grid','native_green_grid_snapshot']]
 units += [ROOT/'src/melee/it/native_item_pool.c',ROOT/'src/melee/it/native_item_link_pool.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',compiler]+list(HERE.glob('*.h'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes={str(path):sha(path) for path in inputs};commands=[]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 runtime_command=[str(compiler),'-std=gnu11','-O2','-DMELEE_NATIVE=1','-DMELEE_ROLLBACK_HOST_STATE=1','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-c',str(runtime),'-o',str(out/'runtime.obj')]
 command=[str(compiler),'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include')]+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 for stage,cmd in [('compile-runtime',runtime_command),('compile',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8');commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-8000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'boundaries':['actual source parser/compiler/storage/events/execution/shared VM','actual GObj creation/procs/deferred retirement and item actor/snapshot owner','native userdata provider and unrelated engine leaves controlled'],'limits':['root immutable entity publication/assets are controlled fixture specs','no full Item asset creation/physics/rendering or live entity parity acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':receipt['summary']}))
if __name__=='__main__':main()
