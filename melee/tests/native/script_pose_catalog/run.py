"""Bounded closed pose selector/catalog/VM test; native assets unavailable."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.exists():p.error('fresh output required')
 out.mkdir(parents=True);spec=importlib.util.spec_from_file_location('exact_body',ROOT/'tests/native/script_item_spawn/extract.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 original=ROOT/'tests/native/script_compile/native_boundaries.c';text=original.read_text();absent=out/'unavailable.c';absent.write_text('#include <melee/ft/native_script_bindings.h>\n'+'\n'.join(module.body(text,n) for n in ['nativeScriptBindings_Context','nativeScriptBindings_Read','nativeScriptBindings_Write','nativeScriptBindingKinetics_Invoke','nativeScriptBindingState_Invoke','nativeScriptBindingSetup_Invoke','nativeScriptBindingCombat_Invoke','nativeScriptBindingTransition_Invoke'])+'\n')
 names=['bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','constants','expression','value','math','source','statement','bytecode','vm','program','binding_effect_resolve']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[HERE/'test.c',absent]
 boundaries=['absorber', 'actions', 'air_inputs', 'air_physics', 'article_combat', 'article_feedback', 'article_lifecycle', 'article_motion', 'article_reference', 'attachments', 'audio', 'capture', 'collision', 'colors', 'common_movement', 'contact', 'cpu_control', 'effect', 'family', 'feedback', 'fighter_hitboxes', 'fighter_items', 'fighter_observations', 'forms', 'ground_controls', 'guard', 'held_state', 'hitbox_create', 'hitlag', 'item', 'item_animation', 'item_collision', 'item_lifecycle', 'item_model', 'item_observations', 'item_resources', 'item_terrain', 'jump', 'ledge', 'model_graph', 'normal_attacks', 'presentation', 'signal', 'spawn', 'stage', 'targets', 'vectors', 'visuals', 'world_feedback']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in boundaries]
 flags=[str(a.compiler),'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref,/errorlimit:0','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include')]
 hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in units+[original,HERE/'run.py',ROOT/'src/melee/ft/native_script_ir.h']};commands=[]
 for stage,cmd in [('compile',flags+list(map(str,units))+['-o',str(out/'test.exe')]),('run',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-7000:])
 for path,digest in hashes.items():
  if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError('Input changed '+path)
 receipt={'status':'PASS','result':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,'scope':'actual closed literal parser/value/Constant-Move/bytecode/VM/topcatalog formal marshalling; explicit rejecting native pose leaf captures exact closed types and raw U64 start','limits':['named graph row7 is controlled resolver input, not actual owned metadata/import','native pose execution and source/recipient ownership tested by root/metadata worker','no assets/renderer/game claim']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
