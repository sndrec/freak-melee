"""Actual owned Item actor/VM/pool Article feedback fixture, no game."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('feedback_source',HERE.parent/'script_form_feedback/extract.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Fresh A: output required')
 out.mkdir(parents=True);hashes={}
 link_extractor_spec=importlib.util.spec_from_file_location('link_extract',ROOT/'tests/native/script_item_spawn/extract.py');link_extract=importlib.util.module_from_spec(link_extractor_spec);link_extractor_spec.loader.exec_module(link_extract)
 link_source=ROOT/'src/melee/it/native_article.c'
 (out/'actual_links.h').write_text('\n\n'.join(link_extract.body(link_source.read_text(),name) for name in ['linked_item','nativeArticle_Link','nativeArticle_Unlink','nativeArticle_LinkReciprocal','nativeArticle_LinkId','nativeArticle_CopyLinkCollision'])+'\n')
 hashes[str(link_source)]=hashlib.sha256(link_source.read_bytes()).hexdigest()

 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_vm','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_pose_metadata','native_fighter_pose_path','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_execution','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_actor_access','native_callback_lifetime','native_script_binding_signal','native_script_binding_item_links','native_script_binding_signal_resolve','native_script_fighter_actor','native_script_fighter_program','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_effect_program','native_script_metadata']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[ROOT/'src/melee/it/native_script_item_actor.c',ROOT/'src/melee/it/native_script_item_snapshot.c']
 units += [ROOT/'src/melee/gm'/(name+'.c') for name in ['native_match_pool','native_match_snapshot_pool','native_rollback_state']]
 units += [ROOT/'src/sysdolphin/baselib'/(name+'.c') for name in ['gobj','gobjinit','gobjplink','gobjproc','gobjgxlink','gobjuserdata','gobjobject','native_gobj_pool_storage','dat']]
 absent=['script_effect_actor_absent','script_bindings_unavailable_actions', 'script_bindings_unavailable_item_resources','script_bindings_unavailable_effect','script_bindings_unavailable_spawn','script_receiver_unavailable_identity','script_bindings_unavailable_common_movement','script_bindings_unavailable_common_properties','script_bindings_unavailable_visuals','script_bindings_unavailable_pose','script_bindings_unavailable_feedback','script_bindings_unavailable_article_feedback','script_bindings_unavailable_jump','script_bindings_unavailable_forms','script_bindings_unavailable_ledge','script_bindings_unavailable_attachments']
 units += [HERE.parent/(name+'.c') for name in absent]+[HERE/'test.c',HERE/'boundary.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations','native_source_asset_validation','ftcharacter_asset_layout','native_fighter_sound_policy','native_script_binding_article_reference_resolve','native_script_binding_item_model_resolve','native_script_model_metadata']]
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['world_feedback','item_model','article_motion','article_combat','targets','cpu_control','article_lifecycle','vectors','article_reference', 'hitlag', 'stage', 'family']]
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['item_lifecycle','item_collision','ground_controls','item_observations','audio','model_graph','fighter_hitboxes','guard','air_inputs','air_physics','fighter_observations','contact','absorber','item_terrain','capture','collision','held_state']]
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'src/melee/it/native_article_animation_origins.c',ROOT/'tests/native/script_bindings_unavailable_item_animation.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',HERE/'README.md',HERE/'fixture.h',HERE/'link_cases.h',ROOT/'tests/native/script_item_spawn/extract.py',a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'));hashes.update({str(path):sha(path) for path in inputs})
 runtime=ROOT/'src/melee/ft/native_script_runtime.c';units.remove(runtime)
 flags=['-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 command=[str(a.compiler),*flags,'-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',*map(str,units),str(out/'runtime.obj'),'-o',str(out/'test.exe')]
 runtime_command=[str(a.compiler),*flags,'-DMELEE_ROLLBACK_HOST_STATE=1','-c',str(runtime),'-o',str(out/'runtime.obj')]
 commands=[]
 for stage,cmd in [('runtime',runtime_command),('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-12000:])
 for path,digest in hashes.items():
  if sha(Path(path))!=digest:raise RuntimeError('Input changed: '+path)
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,'actual':['complete source compiler/catalog/VM/Item actor/storage','GObj creation/free/pool and source guards','real nested Fighter/Item signal execution and payloads','real generic link operations through source/VM, all slots compared with extracted existing native link bodies'],'controlled':['source entity asset publication metadata','position write field leaf injects caller/recipient lifetime changes; no authored native decisions'],'limits':['no Item DAT constructor/game acceptance or linked child creation','borrowed damage owner is an actual foreign Fighter GObj without an attached source actor']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
