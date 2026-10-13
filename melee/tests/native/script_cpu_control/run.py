"""Hidden grouped CPU destination/scalar and command/selector fixtures."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE
spec=importlib.util.spec_from_file_location('target_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--commands-only',action='store_true');a=p.parse_args();out=a.output.resolve()
 if out.exists():p.error('Fresh output directory required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out)
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_cpu_control','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_receiver_unavailable_identity.c',HERE/'connected.c',HERE/'actor_boundary.c']
 units += [p for p in sorted(HERE.parent.glob('script_bindings_unavailable_*.c')) if p.stem not in {'script_bindings_unavailable_presentation','script_bindings_unavailable_item','script_bindings_unavailable_cpu_control','script_bindings_unavailable_actions','script_bindings_unavailable_item_birth'}]
 units += [ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c',ROOT/'src/melee/db/db_state.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'src/melee/ft/native_cpu_history.c']
 command_names=['bindings','binding_item_resources_catalog','binding_spawn_catalog','binding_properties','binding_cpu_control','constants','expression','value','math','source','statement','bytecode','vm','program','binding_effect_resolve','hitbox_metadata']
 command_units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in command_names]+[ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/ft/native_cpu_history.c',ROOT/'src/melee/gm/native_match_pool.c',HERE/'test.c',HERE/'unavailable.c']
 command_units += [p for p in sorted(HERE.parent.glob('script_bindings_unavailable_*.c')) if p.stem!='script_bindings_unavailable_cpu_control']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=[BASE/'fixture.h',BASE/'extract.py',Path(__file__)]+units+command_units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',a.compiler,ROOT/'tools/native_fighter_script_cpu.c',ROOT/'tools/native_fighter_script_code.c',ROOT/'tools/native_fighter_script_types.c',ROOT/'examples/yoshi_wip.fms',ROOT/'examples/ness_wip.fms',ROOT/'docs/scripting/cpu_history.md',ROOT/'docs/scripting/cpu_policies.md']+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-Werror','-Wno-array-parameter','-Wno-int-to-pointer-cast','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-g','-include',str(ROOT/'src/melee/ft/native_fighter_lifetime.h'),'-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(BASE),'-I'+str(ROOT/'src/melee/it'),'-I'+str(ROOT/'src/melee/ft')]
 commands=[]
 syntax=[str(a.compiler),'-std=c11','-Wall','-Wextra','-Werror','-Wno-array-parameter',*flags[3:],'-fsyntax-only',str(ROOT/'src/melee/ft/native_script_binding_cpu_control.c')]
 fixture_syntax=[str(a.compiler),*flags,'-Wall','-Wextra','-Wno-array-parameter','-Wno-sometimes-uninitialized','-fsyntax-only',str(HERE/'connected.c')]
 compile=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 for stage,cmd in ([('syntax',syntax)] if a.commands_only else [('syntax',syntax),('fixture-syntax',fixture_syntax),('runtime',runtime_command),('compile',compile),('fixture',[str(out/'test.exe')])]):
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 connected_summary=None if a.commands_only else json.loads(r.stdout)
 command_compile=[str(a.compiler),*flags,'-fuse-ld=lld',*map(str,command_units),'-o',str(out/'commands.exe')]
 for stage,cmd in [('command-compile',command_compile),('command-fixture',[str(out/'commands.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':{'connected':connected_summary,'commands_selectors':json.loads(r.stdout)},'source_sha256':hashes,'commands':commands,
 'actual':['frontend/catalog/compiler/Prepare/Publish/Fighter and Article actors/Execution/VM/storage/real GObj pools','exact production property read/write admission/routing/publication and receiver retention; two distinct source banks plus stock receivers','CPUControl backend, unchanged native destination/prepare/append/selector kernels, and complete Fighter/native field comparisons'],
 'controlled':['synthetic imported rig/package metadata and runtime package lookup; static Fighter/Item payload allocation','stage-entry, geometry, RNG, target/item selection and deep planner decision leaves controlled; command cohort program/actor views controlled','unrelated engine leaves unavailable; Fighter payload scene bind boundary'],
 'limits':['no game/pairing/full planner geometry/final CPU scheduling acceptance; planner decision and stage-entry leaves controlled','actual source Article caller VM observes stock/owned level; no stock target VM execution claim; scalar widths use well-formed VM values','selected package/rig loading and native constructor assets are controlled']}
 if a.commands_only:
  receipt['actual']=['actual CPUControl read/write/command backend and native source guard/property routes',
      'unchanged command append and nearest-target selector bodies; typed frontend and VM',
      'actual planner registry admission and post-call payload rejection']
  receipt['controlled']=['synthetic Fighter payloads and owned package/program/storage views',
      'stage/eligibility leaves; dedicated no-op planner leaf records entry and payload retirement',
      'other native leaves explicitly unavailable']
  receipt['limits']=['command/scalar/selector fixture only; connected actor/receiver cohort not run',
      'not real gameplay, full CPU planning, scene lifetime or performance acceptance']
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
