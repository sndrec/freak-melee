"""Exact native vector bodies + typed source/VM dispatch; console only."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.exists():p.error('fresh output required')
 out.mkdir(parents=True);spec=importlib.util.spec_from_file_location('extract',ROOT/'tests/native/script_item_spawn/extract.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
 lb=ROOT/'src/melee/lb/lbvector.c';cross=ROOT/'extern/dolphin/src/dolphin/mtx/vec.c';native=out/'native_vectors.c';native.write_text('#include <melee/lb/lbvector.h>\n#include <placeholder.h>\n#include <macros.h>\n'+ '\n'.join(extract.body(lb.read_text(),name) for name in ['lbVector_Len_xy_accurate','lbVector_Normalize','lbVector_NormalizeXY','lbVector_CrossprodNormalized','lbVector_Angle','lbVector_AngleXY','lbvector_sin','lbvector_cos','lbVector_Rotate'])+'\n'+extract.body(cross.read_text(),'C_VECCrossProduct')+'\n')
 original=ROOT/'tests/native/script_compile/native_boundaries.c';absent=out/'unavailable.c';absent.write_text('#include <melee/ft/native_script_bindings.h>\n'+'\n'.join(extract.body(original.read_text(),n) for n in ['nativeScriptBindings_Context','nativeScriptBindings_Read','nativeScriptBindings_Write','nativeScriptBindingKinetics_Invoke','nativeScriptBindingState_Invoke','nativeScriptBindingSetup_Invoke','nativeScriptBindingCombat_Invoke','nativeScriptBindingTransition_Invoke'])+'\n')
 names=['bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_vectors','constants','expression','value','math','source','statement','bytecode','vm','program','binding_effect_resolve'];units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[HERE/'test.c',native,absent]
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['actions', 'item_resources','feedback','article_feedback','effect','item','presentation','spawn','ledge','jump','forms','pose','common_movement','visuals','signal', 'hitlag', 'stage', 'family']]
 units += [f for f in sorted((ROOT/'tests/native').glob('script_bindings_unavailable_*.c')) if f not in units and f.stem!='script_bindings_unavailable_vectors']
 units.append(ROOT/'src/melee/lb/native_float_math.c')
 inputs=units+[lb,cross,original,HERE/'run.py',HERE/'README.md',ROOT/'src/placeholder.h',ROOT/'extern/dolphin/include/macros.h',ROOT/'src/melee/lb/lbvector.h',ROOT/'src/melee/ft/native_script_bindings.h',ROOT/'src/melee/ft/native_script_binding_vectors.h',ROOT/'src/melee/ft/native_script_ir.h'];hashes={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in inputs};commands=[]
 cmd=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-include',str(ROOT/'src/melee/lb/native_float_math.h'),*map(str,units),'-o',str(out/'test.exe')]
 for stage,command in [('compile',cmd),('run',[str(out/'test.exe')])]:
  r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':command,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-9000:])
 for f,digest in hashes.items():
  if hashlib.sha256(Path(f).read_bytes()).hexdigest()!=digest:raise RuntimeError('Input changed: '+f)
 receipt={'status':'PASS','result':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,'scope':'unchanged native vector/C-cross bodies, real generic binding, typed parser/statement/bytecode/VM','limits':['caller availability/retirement controlled','no actor assets or scene/gameplay claim']};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
