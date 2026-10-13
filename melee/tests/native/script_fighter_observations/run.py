"""Hidden connected Fighter observations fixture."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE
spec=importlib.util.spec_from_file_location('target_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out)
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_fighter_observations','native_script_binding_hitlag','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_receiver_unavailable_identity.c',HERE/'test.c',HERE/'boundary.c']
 units += [p for p in sorted(HERE.parent.glob('script_bindings_unavailable_*.c')) if p.stem not in {'script_bindings_unavailable_presentation','script_bindings_unavailable_item','script_bindings_unavailable_fighter_observations','script_bindings_unavailable_item_birth','script_bindings_unavailable_fighter_items','script_bindings_unavailable_item_observations','script_bindings_unavailable_hitlag','script_bindings_unavailable_common_properties'}]
 units += [ROOT/'src/melee/ft/native_script_binding_common_properties.c']
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c',ROOT/'src/melee/db/db_state.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c',ROOT/'src/melee/ft/native_script_binding_item_animation_prepare.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=[BASE/'fixture.h',BASE/'extract.py',Path(__file__)]+units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',a.compiler,ROOT/'docs/scripting/fighter_observations.md',ROOT/'docs/scripting/movement.md',ROOT/'docs/scripting/held_item_controls.md',ROOT/'docs/scripting/item_observations.md',ROOT/'docs/scripting/hitlag.md',ROOT/'tools/native_fighter_item_script.c',ROOT/'tools/native_fighter_script_primitives.c']+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-g','-include',str(ROOT/'src/melee/ft/native_fighter_lifetime.h'),'-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(BASE),'-I'+str(ROOT/'src/melee/it'),'-I'+str(ROOT/'src/melee/ft')]
 commands=[]
 syntax=[str(a.compiler),'-std=c11','-Wall','-Wextra','-Werror','-Wno-array-parameter',*flags[3:],'-fsyntax-only',*[str(ROOT/'src/melee/ft'/n) for n in ['native_script_binding_fighter_observations.c','native_script_binding_fighter_items.c','native_script_binding_item_observations.c','native_script_binding_hitlag.c','native_script_binding_common_properties.c']]]
 fixture_syntax=[str(a.compiler),*flags,'-Wall','-Wextra','-Wno-array-parameter','-fsyntax-only',str(HERE/'test.c')]
 compile=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 for stage,cmd in [('syntax',syntax),('fixture-syntax',fixture_syntax),('runtime',runtime_command),('compile',compile),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,
 'actual':['frontend/catalog/compiler/Prepare/Publish/Fighter and Article actors/Execution/VM/storage/real GObj pools','exact production property read/write admission/routing/publication and receiver retention; two distinct source banks plus stock receivers','actual Hitlag backend with generic retained physical Active reads; source-owned linked operations remain restricted','Fighter observations backend and exact Item observations / FighterItems Read+Write / physical policy guards; native field/bit comparisons','unchanged native heal-amount function from checkpoint 1e9849431 and exact eligibility expression','actual held ItemView identity admission and source-identical itIsHeavy; five actual common pushback fields and 256 ftCo_80092ED8 formula comparisons'],
 'controlled':['synthetic imported rig/package metadata and runtime package lookup; static Fighter/Item payload allocation and common-data table','unrelated engine leaves unavailable; Fighter payload scene bind boundary','unselected held use-category and animation traversal abort explicitly','voice backend records exact sound/volume/pan/channel arguments without audio; native damage/death fallbacks are controlled leaves; unrelated Combat entry branches unavailable'],
 'new_cases':['canonical action metadata compared with unchanged ft_0892 decoder across 1024 stock/owned words and 512 VM reads','64 exact Item slide-vector read/write cases through production property routing and Article actor/VM; ordinary velocity and unrelated Item fields preserved','read-only metadata and stale/wrong-class/dead-storage admission checks','actual interruption binding helper and selected Combat entry, unchanged ftCommon_8007DB58 and both voice-stop bodies; 64 native/script callback combinations through direct routing and interpreter','native pointer overrides script registration; source detachment and actual interpreted callback fault reject result publication'],
 'limits':['no game/stat-accounting/landing execution acceptance or full pickup search/inventory execution','actual owned caller VM observes stock targets; no stock target VM execution claim','selected package/rig loading and native constructor assets are controlled']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
