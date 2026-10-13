"""Hidden isolated callback velocity compiler/actor/VM fixture."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE
spec=importlib.util.spec_from_file_location('target_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out)
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_targets','native_script_binding_setup','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'tests/native/script_target_bone_unavailable.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 units += [HERE.parent/'script_receiver_unavailable_identity.c',HERE.parent/'script_effect_actor_absent.c',HERE/'test.c',out/'boundary.c',HERE/'leaves.c']
 units += [p for p in sorted(HERE.parent.glob('script_bindings_unavailable_*.c')) if p.stem not in {'script_bindings_unavailable_targets','script_bindings_unavailable_item','script_bindings_unavailable_presentation','script_bindings_unavailable_item_birth'}]
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=[BASE/'fixture.h',BASE/'extract.py',Path(__file__)]+units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',ROOT/'docs/scripting/callback_slots.md',ROOT/'docs/scripting/motion_callbacks.md',ROOT/'examples/link_full.fms',ROOT/'examples/young_link_full.fms',a.compiler]+list(out.glob('*.inc'))+list(out.glob('*.c'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-g','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(BASE)]
 commands=[]
 syntax=[str(a.compiler),'-std=c11','-Wall','-Wextra','-Werror','-Wno-array-parameter',*flags[3:],'-fsyntax-only',str(ROOT/'src/melee/ft/native_script_binding_setup.c')]
 compile=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 for stage,cmd in [('syntax',syntax),('runtime',runtime_command),('compile',compile),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,'reference_revision':extractor.REFERENCE,'actual':['complete compiler/catalog/VM and Fighter/Item actors/storage and native target snapshots','real GObj pool generations/free and selected-source guards','actual ftCommon_8007F76C native callback and real callback actor storage','actual accessory scheduler compared with pinned prechange native ordering; typed instance bindings, pure matches, NewLife, motion retention and snapshots','actual authored division faults; physical native-callback retirement/reuse and retained caller/program/storage continuation fences'], 'controlled':['selected immutable package/rig facts and publication getters; real actor publication','source guard test observation injects target/caller mutations; pure reads have no callbacks','native accessory callbacks and JObj matrix-dirty leaf record scheduling and translation; callback 1 explicitly injects lifetime/metadata changes'], 'limits':['no DAT constructor/game acceptance','source publication metadata is controlled, not actual runtime loading','no renderer or complete animation/motion entry; no catch-policy/tether acceptance','metadata mutation is an explicit callback-boundary test, not normal immutable-package behavior']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
