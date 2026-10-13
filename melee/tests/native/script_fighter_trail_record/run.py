"""Connected actual source Fighter/Execution/trail record comparison; hidden isolated console."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.parent/'script_fighter_scheduler'
spec=importlib.util.spec_from_file_location('scheduler_extract',BASE/'extract_scheduler.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out/'actual_scheduler.h');compiler=a.compiler.resolve()
 spec2=importlib.util.spec_from_file_location('trail_extract',HERE/'extract.py');extra=importlib.util.module_from_spec(spec2);spec2.loader.exec_module(extra);hashes.update(extra.extract(ROOT,out))
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_metadata','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_fighter_program','native_script_fighter_actor','native_script_effect_program','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_setup']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/ft/native_script_binding_spawn_resolve.c',ROOT/'src/melee/ft/native_script_binding_effect_resolve.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c',ROOT/'src/sysdolphin/baselib/dat.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += [HERE/'test.c',out/'boundary.c',HERE.parent/'script_bindings_unavailable_feedback.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['common_properties','cpu_control','targets','item_model','article_combat','world_feedback','vectors','article_reference','article_lifecycle','article_motion', 'hitlag', 'stage']]
 units += [ROOT/'src/melee/ft/native_script_binding_article_reference_resolve.c',ROOT/'src/melee/ft/native_script_binding_signal_resolve.c',ROOT/'tests/native/script_bindings_unavailable_signal.c',ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c']

 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_hitboxes.c',ROOT/'src/melee/ft/native_fighter_trail.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_held_items.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_item_lifecycle.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_absorber.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_ground_controls.c',ROOT/'tests/native/script_bindings_unavailable_item_observations.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'tests/native/script_bindings_unavailable_item_animation.c']
 unavailable=set(['item','item_birth','kinetics','state','combat','presentation','transition','spawn','actions','effect','jump','forms','ledge','setup'])
 for leaf in sorted((ROOT/'tests/native').glob('script_bindings_unavailable_*.c')):
  family=leaf.stem.replace('script_bindings_unavailable_','')
  if family not in unavailable and leaf not in units:units.append(leaf)
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[BASE/'boundary.h',HERE/'run.py',HERE/'extract.py',BASE/'extract_scheduler.py',HERE.parent/'script_form_feedback/extract.py',HERE.parent/'script_item_scheduler/extract_scheduler.py',HERE.parent/'script_kinetics/extract_primitives.py',compiler]+[HERE/'README.md',ROOT/'src/melee/ft/ftafterimage.c']+list(HERE.glob('*.h'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs});commands=[]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-include',str(ROOT/'src/melee/ft/native_fighter_lifetime.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(BASE)]
 runtime_command=[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 command=[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0']+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 # Existing SDK MTX redeclaration and unchanged stock bone/count comparison.
 strict_command=[str(compiler)]+common+['-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-sign-compare','-fsyntax-only',str(HERE/'test.c')]
 for stage,cmd in [('strict-fixture',strict_command),('compile-runtime',runtime_command),('compile',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8');commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-12000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,
          'boundaries':['actual frontend/index/typed parameter preparation/bytecode/Fighter Prepare/publication/VM/Execution/receiver and actor',
                        'actual afterimage read_trail, class ancestry and fourteen-field validation; unchanged canonical Link/Young Link trail bodies',
                        'actual GObj fixed pool allocation/free and retained native-generation/scene-epoch checks',
                        'selected source package/data and unrelated engine operations controlled by frozen scheduler boundary'],
          'limits':['No DAT import, game, AI scene, class bootstrap, renderer, sword-item path, or complete Link/Young Link acceptance',
                    'Controlled selected package, ftLk sword attributes, native part records and initialized class metadata',
                    'Retirement/error injection is a controlled pure observation leaf; no motion/renderer or sword-item path',
                    'Unrelated native binding families explicitly reject']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
