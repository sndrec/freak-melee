"""Generic direct feedback ABI/identity fixture; no game/global build."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from extract import extract
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
 hashes=extract(ROOT,out);compiler=a.compiler.resolve()
 units=[ROOT/'src/melee/ft/native_script_binding_item_resources_catalog.c', ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c', HERE/'test.c',ROOT/'src/melee/ft/native_script_binding_feedback.c',ROOT/'src/melee/ft/native_script_effect_dispatch.c',ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/gm/native_match_pool.c']
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',HERE/'extract.py',compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'));hashes.update({str(p):sha(p) for p in inputs})
 command=[str(compiler),'-std=gnu11','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+list(map(str,units))+['-o',str(out/'test.exe')];commands=[]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-10000:])
 if any(sha(Path(p))!=v for p,v in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,'boundaries':['actual new feedback/classifier, complete actual catalog descriptors and source-layout/physical guards, actual part mapping and native pool allocate/free/reuse','all65536 IDs compared to actual efSync/Alt/Async literal argument cases; scalar immediate-use proof','package/program/storage views and native effect/wind/sample leaves controlled'],'limits':['no renderer/game/native particle construction acceptance','no full source candidate import/VM scheduler in this fixture','unverified void*, forwarded va_list, nonmatching/multiple argument and retained-pointer contracts rejected']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
