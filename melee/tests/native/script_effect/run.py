"""Connected source->publication->actual EF constructor/scheduler/VM fixture."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('effect_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 hashes=extractor.extract(ROOT,out);compiler=a.compiler.resolve()/'clang.exe'
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_spawn','binding_spawn_resolve','binding_properties','binding_effect','binding_effect_resolve','compile', 'binding_item_resources_resolve','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','fighter_actor','fighter_program','item_program', 'resource_metadata','metadata']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot','native_script_item_spawn']]
 units += [ROOT/'src/melee/ef'/(n+'.c') for n in ['native_script_effect_actor','native_script_effect_queue','native_script_effect_snapshot','native_model_effect']]
 units += [ROOT/'src/melee/ft/native_script_effect_program.c',ROOT/'src/melee/lb/native_model_joint.c']
 units += [ROOT/'src/melee/ft/native_script_hitbox_metadata.c',ROOT/'tests/native/script_bindings_unavailable_hitbox_create.c']
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state','native_match_arena','native_match_scene']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 units += [HERE/'test.c',out/'boundary.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['item_animation','world_feedback','item_lifecycle','article_motion','ground_controls','item_model','audio','model_graph','item_observations','article_combat','fighter_hitboxes','targets','cpu_control','colors','fighter_items','guard','air_inputs','air_physics','fighter_observations','contact','item_terrain','item_collision','capture','collision','held_state','held_items','normal_attacks','absorber']]
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_script_binding_item_animation_prepare','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_binding_item_model_resolve','native_script_model_metadata','native_source_declarations','native_source_asset_validation','ftcharacter_asset_layout','native_fighter_sound_policy','native_source_motion_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_article_reference.c',ROOT/'tests/native/script_bindings_unavailable_signal.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['article_lifecycle','vectors','feedback','jump','forms','ledge']]
 units += [ROOT/'tests/native/script_bindings_unavailable_attachments.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_presentation.c']
 units += [ROOT/'src/melee/ft/native_script_effect_dispatch.c']
 units += [ROOT/'src/melee/it'/(name+'.c') for name in ['native_article_animation_demand','native_article_animation_texture','native_article_animation_origins']]
 units += [ROOT/'src/melee/gr'/(name+'.c') for name in ['native_ground_pool','native_green_grid','native_green_grid_snapshot']]
 units += [ROOT/'src/melee/it/native_item_pool.c',ROOT/'src/melee/it/native_item_link_pool.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',HERE/'extract.py',compiler]+list(HERE.glob('*.h'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs});commands=[]
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(HERE),'-I'+str(ROOT/'tests/native/script_item_spawn'),'-I'+str(ROOT/'tests/native/script_item_scheduler')]
 syntax_units=[ROOT/'src/melee/ef'/(n+'.c') for n in ['native_script_effect_actor','native_script_effect_queue','native_script_effect_snapshot']]+[ROOT/'src/melee/ft'/(n+'.c') for n in ['native_script_effect_program','native_script_binding_effect','native_script_binding_effect_resolve']]
 stages=[('syntax-owned',[str(compiler)]+common+['-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only']+list(map(str,syntax_units)))];objects=[]
 for name in ['runtime','fighter_program']:
  unit=ROOT/'src/melee/ft'/('native_script_'+name+'.c');units.remove(unit);obj=out/(name+'.obj');objects.append(obj)
  stages.append(('compile-'+name,[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(unit),'-o',str(obj)]))
 command=[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0']+list(map(str,units+objects))+['-o',str(out/'test.exe')]
 stages += [('compile',command),('fixture',[str(out/'test.exe')])]
 for stage,cmd in stages:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-9000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'boundaries':['actual frontend/metadata/storage/code/immutable preparation+publication/shared receiver VM','actual public EF dispatch prefix/Tornado efAlt branch/model constructors/scheduler/update/userdata teardown','actual GObj/Fighter/Item/Effect owners and native joint operations','controlled imported descriptors/model loader/allocator/matrix/draw/audio/platform leaves'],'limits':['other gfx branches unavailable in fixture','no game/retail model/full render or whole match snapshot claim']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':receipt['summary']}))
if __name__=='__main__':main()
