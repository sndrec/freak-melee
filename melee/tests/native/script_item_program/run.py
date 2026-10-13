"""Production Item metadata/preparation/publication fixture; no game launch."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def body(text,name):
 match=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not match:raise RuntimeError('missing actual body '+name)
 end=text.index('{',match.start())+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[match.start():end]
def extract(root,out):
 path=Path(root)/'src/melee/ft/native_script_execution.c';data=path.read_bytes();text=data.decode()
 parts=[body(text,name) for name in ['error','owner_source','nativeScriptExecution_BuildFunctions']]
 recipes=Path(root)/'src/melee/ef/efsync.c';parts.append(body(recipes.read_text(),'efSync_ModelRecipe'))
 Path(out).mkdir(parents=True,exist_ok=True);(Path(out)/'build_functions.inc').write_text('\n\n'.join(parts)+'\n')
 return {str(path):hashlib.sha256(data).hexdigest(),str(recipes):hashlib.sha256(recipes.read_bytes()).hexdigest()}
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 hashes=extract(ROOT,out);compiler=a.compiler.resolve()/'clang.exe'
 names=['native_script_source','native_script_expression','native_script_statement','native_script_program','native_script_bytecode','native_script_value','native_script_math','native_script_constants','native_script_bindings', 'native_script_binding_item_resources_catalog', 'native_script_binding_spawn_catalog','native_script_binding_effect_resolve','native_script_effect_program','native_script_binding_spawn_resolve','native_script_compile', 'native_script_binding_item_resources_resolve','native_script_moves','native_script_move_lowering','native_script_events','native_script_event_payload','native_script_storage','native_script_runtime','native_script_receiver','native_script_binding_receiver','native_script_metadata','native_script_item_program', 'native_script_binding_item_animation_prepare', 'native_script_resource_metadata','native_script_fighter_program']
 units=[ROOT/'src/melee/ft'/(n+'.c') for n in names]+[ROOT/'src/sysdolphin/baselib/dat.c',ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/melee/ft/native_callback_lifetime.c',HERE.parent/'script_bindings_unavailable_actions.c',HERE.parent/'script_bindings_unavailable_actions.c',HERE.parent/'script_bindings_unavailable_effect.c',HERE.parent/'script_bindings_unavailable_spawn.c',HERE/'test.c',HERE/'boundary.c',HERE.parent/'script_receiver_unavailable_registry.c',HERE.parent/'script_receiver_unavailable_identity.c',HERE.parent/'script_receiver_unavailable_pool.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c']
 units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
 units += [ROOT/'src/melee/ft/native_script_fighter_hooks.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_declarations', 'native_source_asset_validation', 'ftcharacter_asset_layout', 'native_fighter_sound_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'src/melee/ft'/(name+'.c') for name in ['native_source_motion_policy']]
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c', ROOT/'src/melee/it/native_article_animation_origins.c']
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 hashes.update({str(path):sha(path) for path in inputs});commands=[]
 command=[str(compiler),'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+list(map(str,units))+['-o',str(out/'test.exe')]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-7000:])
 if any(sha(Path(path))!=expected for path,expected in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'boundaries':['actual frontend/storage/DAT scalar import/entity and Fighter program preparation/publication','exact source execution owner-record builder; runtime dispatch not exercised','controlled actor pin lookups, imported asset descriptors/icons and valid readonly DAT bytes'],'limits':['no renderer/PNG decoding or native Item creation','synthetic valid DAT container; retail entity DAT pass pending if requested']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':receipt['summary']}))
if __name__=='__main__':main()
