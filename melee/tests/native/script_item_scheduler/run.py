"""Actual Item scheduler + actual actor/VM/GObj lifecycle, isolated console only."""
import argparse,hashlib,json,subprocess,importlib.util,sys,struct
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('scheduler_extract',HERE/'extract_scheduler.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def scratch_sections(path):
 data=path.read_bytes();_,count,_,symbols,total,optional,_=struct.unpack_from('<HHIIIHH',data);strings=symbols+total*18;sections=[]
 def name(raw):
  if raw[:4]==b'\0'*4:
   at=strings+struct.unpack_from('<I',raw,4)[0];return data[at:data.index(b'\0',at)].decode()
  if raw.startswith(b'/'):
   at=strings+int(raw[1:].rstrip(b'\0'));return data[at:data.index(b'\0',at)].decode()
  return raw.rstrip(b'\0').decode()
 for i in range(count):sections.append(name(data[20+optional+i*40:28+optional+i*40]))
 found={};i=0
 while i<total:
  at=symbols+i*18;symbol=name(data[at:at+8]);section=struct.unpack_from('<h',data,at+12)[0]
  if symbol in ('interaction_frames','interaction_depth'):
   if section<=0:raise RuntimeError('scratch lacks a defined section')
   found[symbol]=sections[section-1]
  i+=1+data[at+17]
 if len(found)!=2 or any(value.startswith('.mstate') for value in found.values()):raise RuntimeError('interaction scratch entered rollback state: '+str(found))
 return found
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out/'actual_scheduler.h');compiler=a.compiler
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += [HERE.parent/'script_effect_actor_absent.c',HERE.parent/'script_bindings_unavailable_actions.c',HERE.parent/'script_bindings_unavailable_effect.c',HERE.parent/'script_bindings_unavailable_spawn.c',HERE/'test.c',HERE/'boundary.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 # Unselected families reject explicitly; the actual Item scheduler, actor and
 # VM still execute the selected lifecycle/phase operations in boundary.c.
 absent=['item_animation','world_feedback','item_lifecycle','item_collision',
  'article_motion','ground_controls','item_model','audio','model_graph',
  'item_observations','fighter_hitboxes','article_combat','targets','cpu_control',
  'guard','air_inputs','air_physics','fighter_observations','contact',
  'article_lifecycle','absorber','item_terrain','capture','collision','held_state',
  'held_items','ledge','forms','jump','vectors','signal','article_reference']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in absent]
 units += [ROOT/'tests/native/script_bindings_unavailable_feedback.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in [
  'native_script_binding_item_model_resolve','native_script_model_metadata',
  'native_script_binding_article_reference_resolve','native_script_binding_signal_resolve']]
 units += [ROOT/'src/melee/it'/(name+'.c') for name in [
  'native_article_animation_demand','native_article_animation_texture','native_article_animation_origins']]
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',HERE/'extract_scheduler.py',compiler]+list(HERE.glob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs});commands=[]
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 runtime_command=[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 interaction_command=[str(compiler)]+common+['-fno-zero-initialized-in-bss','-c',str(ROOT/'src/melee/it/it_26B1.c'),'-o',str(out/'interaction.obj')]
 command=[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref']+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 for stage,cmd in [('compile-runtime',runtime_command),('compile-interaction-object',interaction_command),('compile',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8');commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-12000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'scratch_sections':scratch_sections(out/'interaction.obj'),'input_sha256':hashes,'commands':commands,'boundaries':['actual Item scheduler/state setup/removal/constructor bodies','actual source compiler/storage/events/VM and actor/GObj deferred lifecycle','asset/graphics/audio/collision leaves and immutable spec provider controlled'],'limits':['no game launch or full engine scene/asset parity','Workshop shared protocol covered separately by root actual-body fixture']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
