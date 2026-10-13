"""Indexed signal source compiler and guarded native dispatch census; no engine."""
import argparse,hashlib,importlib.util,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);spec=importlib.util.spec_from_file_location('extract',ROOT/'tests/native/script_item_spawn/extract.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
 original=ROOT/'tests/native/script_compile/native_boundaries.c';absent=out/'unavailable.c';absent.write_text('#include <melee/ft/native_script_bindings.h>\n'+'\n'.join(extract.body(original.read_text(),n) for n in ['nativeScriptBindings_Context','nativeScriptBindings_Read','nativeScriptBindings_Write','nativeScriptBindingKinetics_Invoke','nativeScriptBindingState_Invoke','nativeScriptBindingSetup_Invoke','nativeScriptBindingCombat_Invoke','nativeScriptBindingTransition_Invoke'])+'\n')
 names=['pose_metadata','compile', 'binding_item_resources_resolve','moves','move_lowering','binding_signal_resolve','binding_spawn_resolve','binding_effect_resolve','runtime','receiver','binding_receiver','constants','execution','event_payload','events','storage','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','value','math','expression','source','statement','program','bytecode','vm']
 units=[ROOT/'src/melee/ft'/('native_script_'+n+'.c') for n in names]+[ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/ft/native_fighter_pose_path.c',ROOT/'src/melee/gm/native_match_pool.c',HERE/'test.c',absent]
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['actions', 'item_resources','feedback','article_feedback','effect','item','presentation','spawn','ledge','jump','forms','pose','common_movement','visuals', 'hitlag', 'stage', 'family']]
 units += [ROOT/'tests/native'/('script_receiver_unavailable_'+n+'.c') for n in ['identity','pool','registry']]
 headers=[ROOT/'src/melee/ft'/n for n in ['native_script_bindings.h','native_script_binding_signal.h','native_script_binding_signal_resolve.h','native_script_binding_spawn.h','native_script_compile.h','native_script_ir.h']]
 hashes={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in units+headers+[original,HERE/'run.py']}
 cmd=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,units),'-o',str(out/'test.exe')];commands=[]
 for stage,command in [('compile',cmd),('run',[str(out/'test.exe')])]:
  r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':command,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-9000:])
 for f,digest in hashes.items():
  if hashlib.sha256(Path(f).read_bytes()).hexdigest()!=digest:raise RuntimeError('Input changed: '+f)
 result={'status':'PASS','result':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,'scope':'all probe bodies typed/compiled, exact Article signal owner+function identity lowering, static signal recursion rejection, accepted-bank failure preservation, VM Text/numeric native marshalling','limits':['Article metadata states/imported assets controlled strictly by fixture validator; no asset activation','actual native signal runtime rejects explicitly; no live actor delivery claim']};(out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result['result']))
if __name__=='__main__':main()
