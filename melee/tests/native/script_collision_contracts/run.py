"""Actual CPU/common action source comparisons; no game launch."""
import argparse,hashlib,json,subprocess,importlib.util,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('actual_body',HERE.parent/'script_item_spawn/extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: archive required')
 out.mkdir(parents=True);pieces=[];hashes={}
 path=ROOT/'src/melee/ft/ft_081B.c';text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
 pieces=[extractor.body(text,n) for n in ['ft_80083090_inline','ft_800831CC','ft_80083B68','ft_80084104']]
 (out/'actual_collision.h').write_text('\n\n'.join(pieces)+'\n')
 units=[HERE/'test.c',ROOT/'src/melee/ft/native_script_binding_collision.c',ROOT/'src/melee/ft/native_callback_lifetime.c']
 command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr)
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-6000:])
 hashes.update({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in units+[HERE/'run.py']})
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'compiler':command,'boundaries':['actual ft_800831CC/83090 collision ordering and standard ground/air bodies','actual native registry; collision/contact/environment/landing/wall/cliff leaves controlled','other action/collision/effect entry points explicitly unavailable'],'game_acceptance':False}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
