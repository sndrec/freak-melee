"""Bounded hidden ABI inventory and actual helper-copy fixture."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from extract import extract
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
 hashes,positions=extract(ROOT,out);compiler=a.compiler.resolve();units=[HERE/'test.c',ROOT/'src/melee/ft/native_script_effect_dispatch.c']
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();inputs=units+[HERE/'run.py',HERE/'extract.py',HERE/'README.md',compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'));hashes.update({str(p):sha(p) for p in inputs})
 command=[str(compiler),'-std=gnu11','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+list(map(str,units))+['-o',str(out/'test.exe')];commands=[]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-8000:])
 if any(sha(Path(p))!=v for p,v in hashes.items()):raise RuntimeError('input changed')
 proof=json.loads((out/'proof.json').read_text())
 receipt={'status':'PASS','summary':json.loads(r.stdout),'native_reference':'50f2b476b','position_ids':positions,'position_requirements':proof['position_requirements'],'position_scalar_requirements':proof['position_scalar_requirements'],'position_two_scalar_requirements':proof['position_two_scalar_requirements'],'input_sha256':hashes,'commands':commands,'boundaries':['actual production classifier vs complete literal switch cases and numeric generator branch source proof for65536IDs','exact actual efLib CreateGenerator/AttachPos/TranslateFacingDir copy bodies and inline JObj translation; allocation/model nodes controlled','Attach_Scale argument/refcount/root-scale and all six forwarded facing-helper joint/scalar source proofs','six erased single-JObj Attach_Scale_FacingDir model cases: typed callee, native attachment, copied creator-root scale, live Fighter-parent facing update','Position and PositionScalar context flags derived from actual creator-root reads, common model helper/update initialization and assigned native callback bodies','34 PositionScalar paths: mutually exclusive scalar reads, shared rotation goto and direct or immediate helper pointer consumption','one PositionTwoScalar recipe: exact ordered Vec3/F32/F32 reads; immediate facing and rotation consumption; no creator-root dependency','JointOffset recipe: typed JObj/Vec3 callee, actual helper offset copy and attachment/callback identity'],'limits':['no real effect/particle constructor, renderer/game or runtime source retirement acceptance','unknown forwarding, extra arguments and unaudited erased void* remain rejected; four exact TwoJoint cases admitted; negative/out-of-range IDs rejected; numeric generators use exact copied Vec3 ABI']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
