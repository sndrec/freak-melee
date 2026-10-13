"""Actual linked-hitlag compiler/actor/VM and native field contract cohort."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BASE=HERE.parent/'script_fighter_scheduler'
spec=importlib.util.spec_from_file_location('scheduler_extract',BASE/'extract_scheduler.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def extract(root,out):
 spec=importlib.util.spec_from_file_location('bodies',root/'tests/native/script_form_feedback/extract.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 hashes={}
 def read(relative):
  p=root/relative;hashes[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest();return p.read_text()
 props=read('src/melee/ft/native_script_binding_properties.c')
 guards='\n'.join(m.body(props,n) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout']).replace('error(', 'hitlag_guard_error(')
 boundary=read('tests/native/script_fighter_scheduler/boundary.c')
 boundary=boundary.replace(m.body(boundary,'nativeScriptBindings_SourceAlive'),'')
 read_actual=m.body(props,'nativeScriptBindings_Read');start=read_actual.index('        case NativeScriptBinding_HitlagActive:');end=read_actual.index('            break;',start)+len('            break;')
 branch=read_actual[start:end]
 boundary=boundary.replace(m.body(boundary,'nativeScriptBindings_Read'),read_actual[:read_actual.index('{')+1]+'\n(void)selectors;if(selector_count||!result)return false;NativeScriptValue value;switch(semantic){\n'+branch+'\ndefault:return false;}*result=value;return true;\n}')
 (out/'boundary.c').write_text('#include <melee/ft/native_script_binding_hitlag.h>\n#include <melee/ft/native_source_package.h>\n#include <melee/ft/ftdat.h>\n'+boundary+'\n'+guards+'\n')
 parts=[]
 for path,names in [('src/melee/ft/fighter.c',['Fighter_UnkSetFlag_8006CFBC']),('src/melee/ft/native_fighter_relations.c',['nativeFighterRelations_Link','nativeFighterRelations_LinkedId']),('src/melee/ft/ftlib.c',['ftLib_8008741C','ftLib_80087460'])]:
  text=read(path);parts.extend(m.body(text,n) for n in names)
 (out/'native_hitlag.inc').write_text('\n\n'.join(parts)+'\n')
 return hashes

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);hashes=extractor.extract(ROOT,out/'actual_scheduler.h');compiler=a.compiler
 hashes.update(extract(ROOT,out))
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_metadata','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_fighter_program','native_script_fighter_actor','native_script_effect_program','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_setup']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/ft/native_script_binding_spawn_resolve.c',ROOT/'src/melee/ft/native_script_binding_effect_resolve.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c',ROOT/'src/sysdolphin/baselib/dat.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage']]
 units += [HERE/'test.c',out/'boundary.c',HERE.parent/'script_bindings_unavailable_feedback.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['common_properties','cpu_control','targets','item_model','article_combat','world_feedback','vectors','article_reference','article_lifecycle','article_motion']]
 units += [ROOT/'src/melee/ft/native_script_binding_article_reference_resolve.c',ROOT/'src/melee/ft/native_script_binding_signal_resolve.c',ROOT/'tests/native/script_bindings_unavailable_signal.c',ROOT/'src/melee/ft/native_script_binding_item_model_resolve.c',ROOT/'src/melee/ft/native_script_model_metadata.c']

 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 units += [ROOT/'src/melee/ft/native_script_binding_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_fighter_hitboxes.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_held_items.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_item_lifecycle.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_absorber.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_ground_controls.c',ROOT/'tests/native/script_bindings_unavailable_item_observations.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'tests/native/script_bindings_unavailable_item_animation.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['guard','air_inputs','air_physics','fighter_observations','held_state','contact']]
 units += [ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[BASE/'boundary.h',HERE/'run.py',BASE/'extract_scheduler.py',HERE.parent/'script_item_scheduler/extract_scheduler.py',HERE.parent/'script_kinetics/extract_primitives.py',compiler]+[HERE/'README.md',ROOT/'docs/scripting/hitlag.md']+list(HERE.glob('*.h'))+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 inputs += [out/'boundary.c',out/'native_hitlag.inc',out/'actual_scheduler.h',HERE.parent/'script_form_feedback/extract.py']
 hashes.update({str(path):sha(path) for path in inputs});commands=[];summaries={}
 (out/'input_sha256.json').write_text(json.dumps(hashes,indent=2)+'\n')
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 common=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-include',str(ROOT/'src/melee/ft/native_fighter_lifetime.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-I'+str(BASE)]
 runtime_command=[str(compiler)]+common+['-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 command=[str(compiler)]+common+['-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref','-Wl,/errorlimit:0']+list(map(str,units))+[str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 strict=[str(compiler)]+common+['-fsyntax-only','-Wall','-Wextra','-Werror','-Wno-array-parameter',str(ROOT/'src/melee/ft/native_script_binding_hitlag.c')]
 for stage,cmd in [('strict-backend',strict),('compile-runtime',runtime_command),('compile',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8');commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-12000:])
  if stage=='fixture':summaries[stage]=json.loads(result.stdout)
 changed={path:{'before':expected,'after':sha(Path(path))} for path,expected in hashes.items() if sha(Path(path))!=expected}
 (out/'changed_inputs.json').write_text(json.dumps(changed,indent=2)+'\n')
 if changed:raise RuntimeError('input changed: '+', '.join(changed))
 receipt={'status':'PASS','summary':summaries,'input_sha256':hashes,'commands':commands,
          'boundaries':['Actual source/index/compile/Prepare/Fighter actor/VM/catalog and Hitlag backend',
                        'Actual GObj pool and exact extracted SourceAlive/SourceFighterLayout; retained native and actor peer lifetimes',
                        'Exact native CFBC release setter, Link/LinkedId reference, and ftLib creation-ID helpers',
                        'Property routing is the exact Hitlag case from properties.c; unrelated property/native leaves reject or use the scheduler boundary'],
          'limits':['No whole match, renderer, importer, hitlag timing/recursive scheduler acceptance or page-history proof',
                    'Package/data/motion metadata controlled; graph/runtime/actor algorithms are actual production code',
                    'Native relation helpers perform no callbacks; negatives cover prior retirement/fault/schema/reuse, not invented synchronous callbacks']}


 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
