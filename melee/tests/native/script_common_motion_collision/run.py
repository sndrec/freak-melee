"""Hidden isolated ground/air conversion and nudge collision fixture."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('target_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out)
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata','native_script_binding_actions','native_script_binding_common_movement','native_script_binding_collision','native_script_binding_item_model_resolve','native_script_model_metadata','native_source_bank']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 absent=['script_receiver_unavailable_identity','script_bindings_unavailable_targets', 'script_bindings_unavailable_item_resources','script_bindings_unavailable_cpu_control','script_bindings_unavailable_article_lifecycle','script_effect_actor_absent','script_bindings_unavailable_effect','script_bindings_unavailable_spawn','script_bindings_unavailable_common_properties','script_bindings_unavailable_visuals','script_bindings_unavailable_pose','script_bindings_unavailable_feedback','script_bindings_unavailable_article_feedback','script_bindings_unavailable_article_reference','script_bindings_unavailable_signal','script_bindings_unavailable_vectors','script_bindings_unavailable_jump','script_bindings_unavailable_forms','script_bindings_unavailable_ledge']
 units += [HERE.parent/(name+'.c') for name in absent]+[HERE/'test.c',HERE/'boundary.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 units += [out/'state_read.c',out/'properties.c',ROOT/'tests/native/script_bindings_unavailable_article_motion.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_item_model.c',ROOT/'tests/native/script_bindings_unavailable_article_combat.c',ROOT/'tests/native/script_bindings_unavailable_world_feedback.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'tests/native/script_bindings_unavailable_item_animation.c']
 # Close current catalog direct dependencies with explicit unselected leaves.
 selected={'script_bindings_unavailable_actions','script_bindings_unavailable_common_movement','script_bindings_unavailable_collision','script_bindings_unavailable_item','script_bindings_unavailable_item_birth','script_bindings_unavailable_presentation'}
 for path in sorted(HERE.parent.glob('script_bindings_unavailable_*.c')):
  if path.stem not in selected and path not in units:units.append(path)
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',ROOT/'docs/scripting/movement.md',ROOT/'docs/scripting/stage_movement.md',ROOT/'examples/link_full.fms',ROOT/'examples/young_link_full.fms',a.compiler]+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-g','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(HERE),'-I'+str(ROOT/'src/melee/ft')]
 commands=[]
 syntax=[str(a.compiler),'-std=c11','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-unused-variable','-Wno-unused-parameter',*flags[3:],'-fsyntax-only',str(ROOT/'src/melee/ft/native_script_binding_common_movement.c'),str(ROOT/'src/melee/ft/native_script_binding_collision.c'),str(ROOT/'src/melee/ft/ft_081B.c'),str(ROOT/'src/melee/ft/native_script_binding_state.c'),*[str(ROOT/'src/melee/ft/kinds/ftCommon'/(name+'.c')) for name in ['ftCo_Landing','ftCo_HammerLanding','ftCo_HammerWait']]]
 compile=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 fixture_syntax=[str(a.compiler),*flags,'-fsyntax-only',str(HERE/'test.c'),str(HERE/'boundary.c'),str(out/'state_read.c')]
 for stage,cmd in [('syntax',syntax),('fixture-syntax',fixture_syntax),('runtime',runtime_command),('compile',compile),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,'actual':['source compiler/catalog/VM and Fighter actor/storage','real GObj pools and source lifetime guards','exact ground-to-air/nudge/air-contact/AirCatchHit native bodies compared to console branch','actual Landing/Hammer common continuations and native threshold scaling'], 'controlled':['owned package metadata and physical Fighter construction','stage geometry/environment, edge/Fall and ordinary neutral response leaves','motion engine leaf dispatches real owned GroundedMotionSetup/StateChange VM hooks in native order; full motion engine not linked','actual owned Landing/LandingAction hooks use real VM and controlled soft-floor engine leaf to inject bounded faults/retirement'], 'limits':['no retail DAT loading or game acceptance','native stage geometry/environment, renderer and full Fighter_ChangeMotionState internals are not compared','console collision reference calls the same current state entry continuations; validates collision ordering, not stock donor source equivalence']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
