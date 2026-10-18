"""Hidden connected target-bone compiler/actor/native sampling cohort."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);s=importlib.util.spec_from_file_location('extract',HERE/'extract.py');e=importlib.util.module_from_spec(s);s.loader.exec_module(e);hashes=e.extract(ROOT,out)
 names=['source','expression','statement','program','bytecode','vm','value','math','constants','bindings','binding_item_resources_catalog','binding_spawn_catalog','compile','binding_item_resources_resolve','metadata','item_program','resource_metadata','fighter_program','fighter_actor','fighter_hooks','effect_program','moves','move_lowering','events','event_payload','storage','execution','runtime','receiver','binding_receiver','actor_access','pose_metadata','binding_spawn_resolve','binding_effect_resolve','binding_signal_resolve','binding_article_reference_resolve','binding_item_model_resolve','model_metadata','binding_targets','hitbox_metadata','binding_hitbox_create','binding_fighter_hitboxes']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]
 units += [ROOT/'src/melee/ft'/(n+'.c') for n in ['native_callback_lifetime','native_fighter_pose_path','native_source_declarations','native_source_asset_validation','native_fighter_sound_policy','ftcharacter_asset_layout','native_source_motion_policy','native_source_bank']]
 units += [ROOT/'src/melee/ft/native_script_binding_item_animation_prepare.c']
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_script_item_actor','native_script_item_snapshot','native_article_animation_demand', 'native_article_animation_texture','native_article_animation_origins']]
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(n+'.c') for n in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 units += sorted(f for f in (ROOT/'tests/native').glob('script_bindings_unavailable_*.c') if f.name not in ['script_bindings_unavailable_targets.c','script_bindings_unavailable_presentation.c','script_bindings_unavailable_item.c','script_bindings_unavailable_item_birth.c','script_bindings_unavailable_hitbox_create.c','script_bindings_unavailable_fighter_hitboxes.c'])
 birth=ROOT/'tests/native/script_bindings_unavailable_item_birth.c';bs=importlib.util.spec_from_file_location('body',ROOT/'tests/native/script_form_feedback/extract.py');bm=importlib.util.module_from_spec(bs);bs.loader.exec_module(bm)
 text=birth.read_text();(out/'unavailable_birth.c').write_text(text.replace(bm.body(text,'nativeScriptItem_GetCreationValue'),''));units.append(out/'unavailable_birth.c')
 units += [ROOT/'tests/native/script_receiver_unavailable_identity.c',ROOT/'tests/native/script_effect_actor_absent.c',HERE/'test.c',HERE/'boundary.c',out/'native_bodies.c']
 units += [ROOT/'src/melee/ft/native_fighter_rig.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_presentation.c',ROOT/'tests/native/script_graph_observers.c']
 units += [ROOT/'src/melee/gm'/(n+'.c') for n in ['native_match_arena','native_match_scene']]
 units += [ROOT/'src/melee/gr'/(n+'.c') for n in ['native_ground_pool','native_green_grid','native_green_grid_snapshot']]
 units += [ROOT/'src/melee/it'/(n+'.c') for n in ['native_item_pool','native_item_link_pool']]
 sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
 inputs=units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',ROOT/'docs/scripting/target_bone_queries.md',ROOT/'docs/scripting/targets.md',ROOT/'docs/scripting/fighter_hitboxes.md',ROOT/'examples/link_full.fms',ROOT/'examples/young_link_full.fms',a.compiler,birth]+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(f):sha(f) for f in inputs});(out/'input_sha256.json').write_text(json.dumps(hashes,indent=2)+'\n')
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(HERE),'-I'+str(out)]
 commands=[]
 strict_units=[ROOT/'src/melee/ft'/name for name in ['native_script_binding_targets.c','native_script_hitbox_metadata.c','native_script_binding_hitbox_create.c']]
 stages=[('strict',[str(a.compiler),*flags,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',*map(str,strict_units)]),('fixture-strict',[str(a.compiler),*flags,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',str(HERE/'test.c'),str(HERE/'boundary.c')]),('runtime',[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]),('compile',[str(a.compiler),*flags,'-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]),('fixture',[str(out/'test.exe')])]
 for stage,cmd in stages:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-16000:])
 changed={f:{'before':h,'after':sha(Path(f))} for f,h in hashes.items() if sha(Path(f))!=h};(out/'changed_inputs.json').write_text(json.dumps(changed,indent=2)+'\n')
 if changed:raise RuntimeError('inputs changed')
 receipt={'status':'PASS','summary':json.loads(r.stdout),
  'native_references':{'camera_bone_scale_guard':'948cf3ed5','shield_and_ahead_search':'ce94f0a0b','capsule_overlap':'dd5b4561b'},
  'input_sha256':hashes,'commands':commands,
  'actual':['compiler/Prepare/catalog/VM/Fighter+Item actors/storage','physical GObj pools/free/reuse',
   'native target BonePosition, shield/camera/modelscale/guard kernels; lb_8000B1CC/HSD SRT/quaternion/Dolphin matrix math',
   'actual NULL-function expression loader and native dummy classifier',
   'foreign source and stock target geometry; raw descriptor indices, root-offset SRT, parented transforms, distinct SuccessOnly outputs and native nonfinite arithmetic',
   'unchanged native opponent and lockable-Item search references; 128+128 direct/Article VM cases, exclusions, ties, no-target preservation and ECB offsets',
   'ahead-query compiler output aliases/types/counts; wrong source/context/owner/rig rejection',
   'unchanged native capsule kernel and stock bomb-overlap loop: 128 stock + 128 independent rig cases, each direct and Article VM',
   'all four hitbox slots, 0..15 hurt capsules, endpoint caches, radius/depth scaling, collision outputs, no target and malformed rig/count rejection',
   'actual retained attachment/control-owner relations stay distinct from current damage ownership',
   'unchanged native nearest-available search/filter/distance bodies; direct and Fighter VM comparisons across availability, margins, ties, ally/full-width identity, no result and invalid registry/source rejection',
   'immutable hitbox definition compiler, exact integer/geometry/schema/closed-selector admission and Fighter preparation',
   '128 direct and Fighter VM creation comparisons against current native grapple initializer, all four capsules and eight activation/group modes; unchanged group/victim-history and clear-all bodies',
   'actual resource attachment/binding/capture/lookup/detach with real GObj, payload and graph pools; absent, replaced and retired reserved endpoint rejection'],
  'controlled':['selected owned packages/rig metadata and fixed userdata/graph construction',
   'unrelated native bindings explicitly unavailable','team mode, blast limits and device identity supplied as controlled leaves',
   'constraint observer and graph destruction fixed-storage leaves; native sampling continues safely over fixture graph storage',
   'hitbox damage/angle helpers supplied as controlled observers; DebugRom bypasses native stale data',
   'resource node/model construction supplied at a fixture transition; payload removal is a controlled leaf'],
  'limits':['publication proof is not arbitrary destructive HSD callback containment',
   'no full IK/RObj solver, retail graph constructor, game, renderer or paired-role/history acceptance',
   'native scale Vec storage preconstructed; no new allocation claim does not cover existing lazy HSD scale/expression allocations',
   'authored shield-scale hook branch is not exercised; no missile steering/game acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
