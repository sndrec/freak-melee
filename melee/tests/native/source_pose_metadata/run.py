"""Actual bounded owned-pose compiler/metadata contract; console only."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.exists():p.error('Fresh output directory required')
 out.mkdir(parents=True)
 names=['native_script_pose_metadata','native_fighter_pose_path','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_moves','native_script_move_lowering','native_script_binding_effect_resolve','native_script_binding_spawn_resolve','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_constants','native_script_execution','native_script_event_payload','native_script_events','native_script_storage','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_properties','native_script_value','native_script_math','native_script_expression','native_callback_lifetime','native_script_source','native_script_statement','native_script_program','native_script_bytecode','native_script_vm']
 units=[ROOT/'src/melee/ft'/(name+'.c') for name in names]+[HERE/'test.c',HERE/'unavailable.c',HERE.parent/'script_form_transition/unavailable.c',ROOT/'src/melee/gm/native_match_pool.c']
 boundaries=['absorber', 'actions', 'air_inputs', 'air_physics', 'article_combat', 'article_feedback', 'article_lifecycle', 'article_motion', 'article_reference', 'attachments', 'audio', 'capture', 'collision', 'colors', 'common_movement', 'common_properties', 'contact', 'cpu_control', 'effect', 'family', 'fighter_hitboxes', 'fighter_items', 'fighter_observations', 'forms', 'ground_controls', 'guard', 'held_items', 'held_state', 'hitbox_create', 'hitlag', 'item', 'item_animation', 'item_collision', 'item_lifecycle', 'item_model', 'item_observations', 'item_resources', 'item_terrain', 'jump', 'ledge', 'model_graph', 'normal_attacks', 'presentation', 'signal', 'spawn', 'stage', 'targets', 'vectors', 'visuals', 'world_feedback']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in boundaries]
 units += [ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in ['hitbox_metadata','binding_item_model_resolve','binding_article_reference_resolve','binding_signal_resolve','model_metadata']]
 units += [ROOT/'tests/native'/(name+'.c') for name in ['script_receiver_unavailable_identity','script_receiver_unavailable_pool','script_receiver_unavailable_registry','script_source_fighter_absent','script_receiver_unavailable_item_relations','script_kinetics/unavailable_runtime']]
 units += [ROOT/'src/melee/it'/('native_article_animation_'+name+'.c') for name in ['demand','texture']]
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=units+[HERE/'run.py',HERE/'README.md',a.compiler,ROOT/'src/melee/ft/native_source_package.c']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes={str(path):sha(path) for path in inputs}
 command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wl,/errorlimit:0','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,units),'-o',str(out/'test.exe')]
 commands=[]
 for stage,cmd in [('build',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
  (out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8');commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-12000:])
 if any(sha(Path(path))!=digest for path,digest in hashes.items()):raise RuntimeError('Input changed during fixture')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'actual':['Source/Program/Expression compiler, opaque selector literal lowering and Bytecode/VM types','owned pose metadata/table/resolver and actual shared bounded path validator','complete production catalog/constants'],'limits':['Native pose operations and VM callbacks not executed by this metadata fixture','declared facts supplied as controlled metadata; no DAT import/package activation','unrelated native engine functions explicitly unavailable']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
