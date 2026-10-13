"""Compile the actual CMake native-binding fixture in a fresh private directory."""
import argparse, hashlib, importlib.util, json, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent; ROOT=HERE.parents[2]
def module(path,name):
 spec=importlib.util.spec_from_file_location(name,path); result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
 out.mkdir(parents=True)
 kin=module(HERE.parent/'script_kinetics/extract_primitives.py','kinetic_extract')
 state=module(HERE.parent/'script_state/extract_state.py','state_extract')
 combat=module(HERE.parent/'script_combat/extract_combat.py','combat_extract')
 scale=module(HERE/'extract_scale.py','scale_extract')
 hashes=kin.extract(ROOT,out/'script_kinetics_actual.h')['inputs'];hashes.update(state.extract(ROOT,out/'script_state_actual.h')['inputs']);hashes.update(combat.extract(ROOT,out/'script_combat_actual.h')['inputs']);scale.extract(ROOT,out/'common_scale_reference.inc')
 ninja=ROOT/'build/native/build.ninja';line=next(x for x in ninja.read_text().splitlines() if x.startswith('build native_script_native_bindings_test.exe:'))
 prefix='CMakeFiles/native_script_native_bindings_test.dir/'
 units=[ROOT/x[len(prefix):-4] for x in line.split() if x.startswith(prefix) and x.endswith('.c.obj')]
 if not units or HERE/'test.c' not in units:raise RuntimeError('missing configured native-binding sources')
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 inputs=units+[HERE/'README.md',HERE/'run.py',HERE/'extract_scale.py',HERE.parent/'script_kinetics/extract_primitives.py',HERE.parent/'script_state/extract_state.py',HERE.parent/'script_combat/extract_combat.py',ROOT/'src/melee/ft/ftcommon_data.c',ROOT/'src/melee/ft/kinds/ftPeach/ftpeachfloatfall.c',ROOT/'src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c',ROOT/'src/melee/ft/kinds/ftCommon/ftCo_AerialAction.c',ROOT/'src/melee/ft/native_fighter_module.c',ROOT/'tools/native_fighter_script_primitives.c',ROOT/'docs/scripting/movement.md',ninja,a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
 inputs.append(ROOT/'src/sysdolphin/baselib/class.c')
 hashes.update({str(path):sha(path) for path in inputs})
 command=[str(a.compiler),'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-DMELEE_SCRIPT_TEST_CONTEXT=1','-DMELEE_SCRIPT_BINDING_TEST_BOUNDARY=1','-DMELEE_SCRIPT_KINETIC_TEST_BOUNDARY=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+list(map(str,units))+['-o',str(out/'test.exe')]
 commands=[]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-8000:])
 if any(sha(Path(path))!=digest for path,digest in hashes.items()):raise RuntimeError('input changed during fixture')
 receipt={'status':'PASS','summary':json.loads(r.stdout),'input_sha256':hashes,'commands':commands,'scope':'actual configured frontend/VM/property units, extracted native model-scale/root-scale/class-ancestry and signed float-direction queries; rendered-root class/extent admission, F32 bit parity, controlled class-initializer retirement mutations; four pure jump observations with context/readonly/missing-common-data rejection; inherited native/actor/asset leaves are controlled or unavailable','limits':['controlled registered Fighter/JObj class metadata; no owned graph construction or actual retained receiver invocation','no game, renderer, full fighter, resource lifetime or aggregate rollback acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
