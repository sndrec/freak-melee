"""Failed construction child ownership; actual silent native teardown."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE
spec=importlib.util.spec_from_file_location('target_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out)
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 identity_source=HERE.parent/'script_receiver_unavailable_identity.c';identity=out/'unavailable_identity.c'
 identity_text=identity_source.read_text()
 import re
 identity_text=re.sub(r'Fighter_GObj\*\s+nativeItem_Creator\([^)]*\)\s*\{[^}]*\}', '', identity_text)
 identity.write_text(identity_text)
 units += [HERE.parent/'script_effect_actor_absent.c',identity,HERE/'test.c',HERE/'boundary.c']
 units += [p for p in sorted(HERE.parent.glob('script_bindings_unavailable_*.c')) if p.stem!='script_bindings_unavailable_item_birth']
 units += [ROOT/'src/melee/ft/native_fighter_pool.c',ROOT/'src/melee/it/native_item_owner.c']
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c',ROOT/'src/melee/db/db_state.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c',ROOT/'src/melee/ft/native_script_binding_item_animation_prepare.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=[HERE/'extract.py',HERE/'run.py',HERE/'README.md',identity_source,a.compiler,out/'native_bodies.json']+units+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-g','-include',str(ROOT/'src/melee/ft/native_fighter_lifetime.h'),'-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(BASE),'-I'+str(ROOT/'src/melee/it'),'-I'+str(ROOT/'src/melee/ft')]
 commands=[]
 syntax=[str(a.compiler),*flags,'-fsyntax-only',*[str(ROOT/'src/melee/ft'/n) for n in ['fighter.c']]]
 fixture_syntax=[str(a.compiler),*flags,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',str(HERE/'test.c')]
 compile=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 for stage,cmd in [('syntax',syntax),('fixture-syntax',fixture_syntax),('runtime',runtime_command),('compile',compile),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,'native_body_sha256':json.loads((out/'native_bodies.json').read_text()),
 'actual':['source-exact Item_8026A8EC, its four helper bodies and Item_OnUserDataRemove; real it_8026B7F8 and Fighter_Unload_8006DABC','common Item_80267AA8 ownership/admission prefix through creator capture and retiring-constructor refusal','actual owner backend/stamp capture and fixed bounded child sweep','actual frontend/compiler/Prepare/Publish and Fighter/Article actors, Load/Remove/Interaction/Destroyed VM hooks, fixed GObj/Fighter pools'],
 'controlled':['explicit particle-only Article/immutable Fighter metadata and native userdata setup; fixed fixture Item payload pool','command/position property writes create children or transfer/retire/reuse a selected peer synchronously','camera/effect/status/sound/rig leaves; resource cancellation is recorded here and genuine graphs are covered separately by script_item_resources'],
 'limits':['no whole Fighter_Create/Item factory model/attribute/ECB initialization or Player activation acceptance','no genuine rig/resource graph allocation in this teardown cohort; see separate resource receipt','no game/GPU/audio/device operation, arbitrary invalid userdata, or rollback of transferred/unowned Item/effect products']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
