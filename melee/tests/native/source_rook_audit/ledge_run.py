"""Exact generic ledge/cliff/jump body comparisons, hidden standalone compile."""
import argparse,hashlib,json,subprocess,importlib.util,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);spec=importlib.util.spec_from_file_location('actual_body',ROOT/'tests/native/script_item_spawn/extract.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
 sources=[(ROOT/'src/melee/ft/ft_081B.c','ft_CheckGroundAndLedge'),(ROOT/'src/melee/ft/ftcliffcommon.c','ftCliffCommon_80081298'),(ROOT/'src/melee/ft/kinds/ftCommon/ftCo_JumpAerial.c','ftCo_800CB870')]
 (out/'actual_ledge.h').write_text('\n\n'.join(module.body(p.read_text(),name) for p,name in sources)+'\n')
 boundary=ROOT/'tests/native/script_compile/native_boundaries.c';text=boundary.read_text()
 absent=out/'unavailable.c';absent.write_text('#include <melee/ft/native_script_bindings.h>\n'+'\n'.join(module.body(text,name) for name in ['nativeScriptBindings_Context','nativeScriptBindings_Read','nativeScriptBindings_Write','nativeScriptBindingKinetics_Invoke','nativeScriptBindingState_Invoke','nativeScriptBindingSetup_Invoke','nativeScriptBindingCombat_Invoke','nativeScriptBindingTransition_Invoke'])+'\nbool nativeScriptBindingEffect_DescribeCall(void* c,uint32_t s,NativeScriptCallLayout* l,NativeScriptDiagnostic* d){(void)c;(void)s;(void)l;(void)d;return false;}\n')
 units=[HERE/'ledge_test.c',ROOT/'src/melee/ft/native_script_binding_ledge.c',ROOT/'src/melee/ft/native_callback_lifetime.c',absent]
 units += [ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in ['bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','constants','expression','value','math']]
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['actions', 'item_resources','effect','item','presentation','spawn','jump','forms','feedback', 'hitlag', 'stage']]
 flags=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
 hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in units+[p for p,_ in sources]+[HERE/'ledge_run.py',boundary]}
 commands=[]
 for stage,cmd in [('compile',flags+list(map(str,units))+['-o',str(out/'test.exe')]),('run',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-7000:])
 for p,digest in hashes.items():
  if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=digest:raise RuntimeError('Input changed '+p)
 receipt={'status':'PASS','result':json.loads(r.stdout),'source_sha256':hashes,'commands':commands,'boundaries':['actual top-level nativeBindings Invoke routing/formals/result publication + exact ft_CheckGroundAndLedge/checkedcliff/aerial-jump decision bodies','geometry, ledge occupancy/scoring, cliff entry and jump entry leaves controlled; real native source registry','selected SourceFighterLayout absent/rejecting in this raw-stock oracle; its ownership validator tested separately','unrelated native runtime families explicitly unavailable','no actual Rook assets/runtime or game acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
