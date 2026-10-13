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
 for relative,names in [('src/melee/ft/ftcpuattack.c',['ftCo_CpuRangedAttackUnsafe','ftCo_CpuAddAttackCandidate']),('src/melee/ft/ftcommon.c',['ftCommon_UpdateFacing']),('src/melee/ft/ftparts.c',['ftParts_80074A4C']),('src/melee/ft/ftcmdscript.c',['ftCo_800B4880'])]:
  path=ROOT/relative;text=path.read_text();hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
  if 'ftcpuattack' in relative:pieces.append(re.search(r'typedef struct ftCo_CollData \{.*?\} ftCo_CollData;',text,re.S).group())
  pieces += [extractor.body(text,n) for n in names]
 leaves='\nbool mpCheckAll(Vec3* p,int* l,u32* f,Vec3* n,int a,int b,float x,float y,float tx,float ty){(void)p;(void)l;(void)f;(void)n;(void)a;(void)b;(void)x;(void)y;(void)tx;(void)ty;++ray_queries;return obstructed;}\nbool ftCo_800A0FB0(Vec3* p,int* l,u32* f,Vec3* n,int a,int b,int c,float x,float y,float tx,float ty,float z){(void)p;(void)l;(void)f;(void)n;(void)a;(void)b;(void)c;(void)x;(void)y;(void)tx;(void)ty;(void)z;++ground_queries;return grounded;}\n'
 (out/'actual_cpu.h').write_text(leaves+'\n'+'\n\n'.join(pieces)+'\n')
 units=[HERE/'test.c',ROOT/'src/melee/ft/native_script_binding_actions.c',ROOT/'src/melee/ft/native_callback_lifetime.c']
 command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr)
  if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-6000:])
 hashes.update({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in units+[HERE/'run.py']})
 receipt={'status':'PASS','summary':json.loads(r.stdout),'source_sha256':hashes,'compiler':command,'boundaries':['actual CPU command walker/candidate/ranged functions and common facing/visibility source','actual native registry; scene/collision probes and command leaves controlled','other action/collision/effect entry points explicitly unavailable'],'game_acceptance':False}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
