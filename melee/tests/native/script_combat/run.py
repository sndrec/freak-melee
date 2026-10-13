"""Build the production combat frontend/VM fixture without opening windows."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def imported(name,path):
 spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module);return module
state=imported('combat_state',HERE.parent/'script_state/extract_state.py');combat=imported('combat_extract',HERE/'extract_combat.py')
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--compiler',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve()
 if out.exists():parser.error('Use a fresh output directory')
 out.mkdir(parents=True);kin=state.reference.extract(ROOT,out/'script_kinetics_actual.h');st=state.extract(ROOT,out/'script_state_actual.h');co=combat.extract(ROOT,out/'script_combat_actual.h')
 names=['native_script_bindings.c','native_script_constants.c', 'native_script_binding_item_resources_catalog.c', 'native_script_binding_spawn_catalog.c','native_script_binding_properties.c','native_script_binding_kinetics.c','native_script_binding_state.c','native_script_binding_setup.c','native_script_binding_combat.c','native_script_value.c','native_script_math.c','native_script_expression.c','native_callback_lifetime.c','native_script_source.c','native_script_statement.c','native_script_program.c','native_script_bytecode.c','native_script_vm.c']
 units=[ROOT/'src/melee/ft'/n for n in names]+[ROOT/'src/sysdolphin/baselib/random.c',HERE.parent/'script_kinetics/engine_fixture.c',HERE.parent/'script_state/engine_fixture.c',HERE.parent/'script_setup/engine_fixture.c',HERE/'engine_fixture.c',HERE/'test.c']
 units += [ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/melee/gm/native_match_arena.c',HERE.parent/'script_kinetics/unavailable_runtime.c',ROOT/'src/melee/ft/native_script_binding_effect_resolve.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 for leaf in sorted((ROOT/'tests/native').glob('script_bindings_unavailable_*.c')):
  if leaf not in units and leaf.stem.replace('script_bindings_unavailable_','') not in {'combat','kinetics','state','setup'}:units.append(leaf)
 def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
 compiler=args.compiler.resolve()
 pinned=units+[compiler,HERE/'run.py',HERE/'README.md',HERE/'extract_combat.py',HERE.parent/'script_state/extract_state.py',HERE.parent/'script_kinetics/extract_primitives.py',ROOT/'docs/scripting/effect_hitlag.md']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 inputs={str(p):sha(p) for p in pinned};inputs.update(kin['inputs']);inputs.update(st['inputs']);inputs.update(co['inputs']);commands=[];exe=out/'test.exe'
 compile=[compiler,'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-DSCRIPT_FIXTURE_SOURCE_ACTOR=1','-DSCRIPT_FIXTURE_SOURCE_ATTACHED=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 for stage,cmd in [('compile',compile+units+['-o',exe]),('run',[exe])]:
  cmd=list(map(str,cmd));result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n');commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
  if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
 for p,h in inputs.items():
  if sha(Path(p))!=h:raise RuntimeError('Changed input '+p)
 receipt={'status':'PASS','result':json.loads(result.stdout),'inputs':inputs,'commands':commands,'scope':'Actual RNG, damage/status/effect bodies, production source/frontend/bytecode/VM and fixed MatchPool identity. Unattached pause_during_hitlag rejects; attached ownership is tested in script_effect_hitlag. Player/color/presentation/destructor leaves controlled. No game launch or spawning/receiver closure.'}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
