"""Run actual sound/joint bodies through the production presentation catalog/VM."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('presentation_extract',HERE/'extract_presentation.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Use a fresh directory on A:')
 out.mkdir(parents=True);inputs=extractor.extract(ROOT,out/'script_presentation_actual.h')
 names=['native_script_binding_presentation.c','native_script_bindings.c', 'native_script_binding_item_resources_catalog.c', 'native_script_binding_spawn_catalog.c','native_script_binding_properties.c','native_script_constants.c','native_script_value.c','native_script_math.c','native_script_expression.c','native_callback_lifetime.c','native_script_source.c','native_script_statement.c','native_script_program.c','native_script_bytecode.c','native_script_vm.c','native_fighter_sound_policy.c','ftparts_table.c','ftparts_rotation.c']
 units=[ROOT/'src/melee/ft'/n for n in names]+[ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/sysdolphin/baselib/random.c',ROOT/'src/melee/lb/lbjoint_position.c',HERE/'engine_fixture.c',HERE/'unavailable_families.c',HERE/'test.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
 units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
 def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
 inputs.update({str(path):sha(path) for path in units});commands=[];exe=out/'test.exe'
 flags=[a.compiler,'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 for stage,command in [('compile',flags+units+['-o',exe]),('run',[exe])]:
  command=list(map(str,command));result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n');commands.append({'stage':stage,'command':command,'exit':result.returncode})
  if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
 for path,expected in inputs.items():
  if sha(Path(path))!=expected:raise RuntimeError('Input changed '+path)
 receipt={'status':'PASS','result':json.loads(result.stdout),'inputs':inputs,'commands':commands,'scope':'Actual native rotation/world-position/sound mapping/channel bodies, production source/catalog/VM. Audio device and matrix construction leaves controlled; unrelated engine families explicitly unavailable. No game launch.'}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
