"""Hidden isolated compiler/actor/VM and native CPU scheduler fixture."""
import argparse,hashlib,json,subprocess,importlib.util,sys,re
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('cpu_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out)
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_binding_item_model_resolve','native_script_model_metadata','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_article_reference_resolve','native_script_binding_signal_resolve','native_script_fighter_hooks','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 absent=['script_bindings_unavailable_world_feedback', 'script_bindings_unavailable_item_resources.c','script_bindings_unavailable_article_motion','script_bindings_unavailable_article_combat','script_bindings_unavailable_targets','script_receiver_unavailable_identity','script_bindings_unavailable_cpu_control','script_bindings_unavailable_article_lifecycle','script_effect_actor_absent','script_bindings_unavailable_actions','script_bindings_unavailable_effect','script_bindings_unavailable_spawn','script_bindings_unavailable_common_movement','script_bindings_unavailable_common_properties','script_bindings_unavailable_visuals','script_bindings_unavailable_pose','script_bindings_unavailable_feedback','script_bindings_unavailable_article_feedback','script_bindings_unavailable_article_reference','script_bindings_unavailable_signal','script_bindings_unavailable_vectors','script_bindings_unavailable_jump','script_bindings_unavailable_forms','script_bindings_unavailable_ledge']
 absent += ['script_bindings_unavailable_item_model', 'script_bindings_unavailable_item_resources.c']
 units += [HERE.parent/(name+'.c') for name in absent]+[HERE/'test.c',HERE/'boundary.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'tests/native/script_bindings_unavailable_item_animation.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=units+list(HERE.glob('*.py'))+list(HERE.glob('*.h'))+[HERE/'README.md',a.compiler]+list(out.glob('*.inc'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-g','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 commands=[]
 syntax=[str(a.compiler),'-std=c11','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-int-to-pointer-cast','-Wno-switch-bool','-Wno-unused-variable','-Wno-unused-parameter','-Wno-unused-but-set-variable','-Wno-sometimes-uninitialized',*flags[3:],'-fsyntax-only',str(ROOT/'src/melee/ft/kinds/ftCommon/ftCo_0A01.c')]
 compile=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 for stage,cmd in [('syntax',syntax),('runtime',runtime_command),('compile',compile),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 fixture_summary=json.loads(r.stdout)
 # Compare excluded warnings with unchanged HEAD bodies under identical live
 # headers. Messages omit paths/line numbers; new branch diagnostics must not
 # be hidden by a broad warning-category exclusion.
 warning_evidence=[]
 for relative in ['src/melee/ft/kinds/ftCommon/ftCo_0A01.c','src/melee/ft/ftcpuattack.c']:
  original=subprocess.run(['git','show','HEAD:'+relative],cwd=ROOT,capture_output=True,timeout=30,creationflags=subprocess.CREATE_NO_WINDOW)
  if original.returncode:raise RuntimeError('Cannot obtain HEAD warning baseline')
  # The lifetime contract was renamed directly across native call sites. Apply
  # that spelling-only change to the private HEAD body, not to production, so
  # its warning comparison uses the same current declarations without an alias.
  original_digest=hashlib.sha256(original.stdout).hexdigest()
  baseline_bytes=original.stdout.replace(b'native_cpu_lifetime',b'native_fighter_lifetime').replace(b'NativeCpuLifetime',b'NativeFighterLifetime').replace(b'nativeCpuLifetime_',b'nativeFighterLifetime_')
  baseline=out/('baseline_'+Path(relative).name);baseline.write_bytes(baseline_bytes)
  command=[str(a.compiler),'-std=gnu11','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-unused-variable','-Wno-int-to-pointer-cast','-Wno-switch-bool',*flags[3:],'-I'+str((ROOT/relative).parent),'-fsyntax-only']
  messages=[]
  for label,path in [('head',baseline),('current',ROOT/relative)]:
   observed=subprocess.run(command+[str(path)],cwd=ROOT,capture_output=True,text=True,timeout=60,creationflags=subprocess.CREATE_NO_WINDOW)
   log=observed.stdout+observed.stderr;(out/(Path(relative).stem+'_'+label+'_warnings.log')).write_text(log)
   messages.append(set(re.findall(r'error: ([^\n]+)',log)))
  if messages[1]-messages[0]:raise RuntimeError('New excluded native warning: '+repr(messages[1]-messages[0]))
  warning_evidence.append({'unit':relative,'head_body_sha256':original_digest,'warning_body_sha256':sha(baseline),'spelling_only_normalization':'native fighter lifetime rename','head_messages':sorted(messages[0]),'current_messages':sorted(messages[1])})
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,'actual':['source compiler/typed CPU hook table/Prepare/Fighter actor/VM/storage and registered GObj pool','native spacing/grab/attack/stick/tick bodies and exact initializer/recovery source dispatch/default segments','native_fighter_lifetime full physical and logical caller fences'],'controlled':['selected source package/data publication and fixed RNG input','large mode/output common tails, recovery queue and unrelated native leaves','position write injects source retirement/reuse/data/native generation changes'],'limits':['not a complete CPU AI scene or game/DAT acceptance','full27 CPU ABI independently covered by script_cpu_hook_abi','baseline native warnings excluded explicitly in syntax command']}
 receipt['summary']=fixture_summary;receipt['baseline_warning_comparison']=warning_evidence
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
