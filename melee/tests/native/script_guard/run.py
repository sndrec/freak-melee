"""Hidden connected Fighter guard/ShieldHit fixture."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE
spec=importlib.util.spec_from_file_location('target_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out)
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_guard','native_script_binding_combat','native_script_binding_effect','native_script_effect_dispatch','native_script_binding_common_properties','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_receiver_unavailable_identity.c',HERE/'test.c',HERE/'boundary.c']
 units += [p for p in sorted(HERE.parent.glob('script_bindings_unavailable_*.c')) if p.stem not in {'script_bindings_unavailable_presentation','script_bindings_unavailable_item','script_bindings_unavailable_guard','script_bindings_unavailable_effect','script_bindings_unavailable_common_properties','script_bindings_unavailable_item_birth'}]
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'src/melee/ft/native_script_binding_jump.c',ROOT/'src/melee/ft/native_fighter_jump.c']
 units.remove(HERE.parent/'script_bindings_unavailable_jump.c')
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=[BASE/'fixture.h',BASE/'extract.py',Path(__file__)]+units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',ROOT/'docs/scripting/guard.md',ROOT/'docs/scripting/common_policies.md',ROOT/'docs/scripting/combat_visuals.md',ROOT/'docs/scripting/joint_feedback.md',ROOT/'examples/yoshi_wip.fms',a.compiler]+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 inputs += [ROOT/'docs/scripting/reflector.md',ROOT/'docs/scripting/movement.md',ROOT/'docs/scripting/fighter.md',out/'shield_operations_reference.json',out/'guard_flow_reference.json',out/'jump_policy_reference.json']
 hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-g','-include',str(ROOT/'src/melee/ft/native_fighter_lifetime.h'),'-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(BASE),'-I'+str(ROOT/'src/melee/it')]
 commands=[]
 syntax=[str(a.compiler),'-std=c11','-Wall','-Wextra','-Werror','-Wno-array-parameter',*flags[3:],'-fsyntax-only',str(ROOT/'src/melee/ft/native_script_binding_guard.c'),str(ROOT/'src/melee/ft/native_script_binding_common_properties.c'),str(ROOT/'src/melee/ft/native_script_binding_combat.c'),str(ROOT/'src/melee/ft/native_script_binding_effect.c'),str(ROOT/'src/melee/ft/native_script_binding_jump.c')]
 fixture_syntax=[str(a.compiler),*flags,'-Wall','-Wextra','-Wno-array-parameter','-fsyntax-only',str(HERE/'test.c')]
 compile=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 for stage,cmd in [('syntax',syntax),('fixture-syntax',fixture_syntax),('runtime',runtime_command),('compile',compile),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={
  'status':'PASS','summary':json.loads(r.stdout),
  'native_reference':json.loads((out/'reference_source.json').read_text()),
  'shield_operation_reference':json.loads((out/'shield_operations_reference.json').read_text()),
  'guard_flow_reference':json.loads((out/'guard_flow_reference.json').read_text()),
  'jump_policy_reference':json.loads((out/'jump_policy_reference.json').read_text()),
  'source_sha256':hashes,'commands':commands,
  'actual':[
   'actual frontend/compiler/catalog/Prepare/Publish/Fighter actor/Execution/VM/storage/receiver/GObj pools',
   'whole shield transition hook and six named modes through nine complete production scheduler bodies, typed delivery and guarded special-hit continuation',
   'shared guard/common property backends and common release-input body pinned to 829748389',
   'actual raw-part Effect backend and four async queue bodies pinned to 7f6ab2bd6; actual combat backend, capsule initializer/all-status bodies pinned to 7fde757c9; actual guard backend/native constructor and update body/class ancestry; exact ShieldHit ProcessHit branch and damage reset compared with pinned native reference',
   'nine shared guard operations, scale/alpha queries and readonly reflector status through compiled source/actor/VM; operation recipes, native SDI, stock radius/alpha compared with 7ff672421; reflector constructor pinned to same revision',
   'eleven guard input/physics/collision operations and roll/spotdodge hooks through source/actor/VM; complete predicate/entry/ground-movement/priority bodies compared with 926b6e33d',
   'aerial-jump completion, fall-selector and knockdown pose flag through source/actor/VM; complete joint/AObj query and jump/fall-entry bodies compared with 040bc5613'],
  'controlled':[
   'imported rig/package/model class data and shield joint metadata, common tuning data; explicit fixture property routing',
   'native position-write leaf and explicit mutation observer; ordinary shield contact/audio leaves; mutation observers expose successful callback identity substitution separately from VM retirement',
   'native release statistics, special-hit tail, shield-break entry, alpha delivery and sound leaves are recorded; stock/fighter-specific transition leaves abort',
   'guard flow motion initialization, animation, statistics, item throw/drop entry, stage platform query/friction and collision leaves controlled; independent predicate oracle uses controlled powershield/release delivery'],
  'limits':[
   'raw-part async geometry sampling and typed position/scalar recipe delivery are controlled; fixed 16-packet allocator; no particle/model construction, queued arbitrary rig replacement or production allocation-free claim',
   'unrelated combat damage/random/status-query leaves abort if reached; no contact detection/render/game acceptance',
   'no complete motion/animation or terrain collision acceptance, rendered/gameplay acceptance; unrelated ProcessHit hitlag/tails omitted']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
