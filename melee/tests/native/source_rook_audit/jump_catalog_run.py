"""Actual generic jump descriptor/semantic/body/bytecode mapping, no runtime."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True)
 names=['source','expression','statement','value','bytecode','vm','math','constants','bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_effect_resolve','program']
 units=[ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in names]+[HERE/'jump_catalog_test.c',ROOT/'tests/native/script_compile/native_boundaries.c']
 units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['actions', 'item_resources','effect','item','presentation','spawn','ledge','jump','forms','feedback', 'hitlag', 'stage']]
 flags=[str(a.compiler),'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include')]
 hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in units+[HERE/'jump_catalog_run.py',ROOT/'src/melee/ft/native_script_bindings.h']};commands=[]
 for stage,cmd in [('compile',flags+list(map(str,units))+['-o',str(out/'test.exe')]),('run',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-7000:])
 for p,digest in hashes.items():
  if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=digest:raise RuntimeError('Input changed '+p)
 receipt={'status':'PASS','result':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,'scope':'actual catalog/formals, source/statement/IR and bytecode semantic mapping; rejects readonly, wrongBool/context and Void condition; all runtime families explicitly unavailable','game_acceptance':False}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
