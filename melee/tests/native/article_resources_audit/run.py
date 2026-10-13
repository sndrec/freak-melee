"""Read retail DAT and compile isolated probe against already-built native libs."""
import argparse,hashlib,json,re,shutil,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--iso',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
 out.mkdir(parents=True);build=ROOT/'build/native';compiler=a.compiler.resolve()
 with a.iso.open('rb') as f:
  f.seek(0x424);at,size=struct.unpack('>II',f.read(8));f.seek(at);fst=f.read(size);count=struct.unpack_from('>I',fst,8)[0];names=fst[count*12:];data=None
  for i in range(1,count):
   tag,offset,size=struct.unpack_from('>III',fst,i*12);n=tag&0xffffff
   if names[n:names.find(b'\0',n)]==b'PlSk.dat':f.seek(offset);data=f.read(size);break
 if not data:raise RuntimeError('actual PlSk.dat not found')
 dat=out/'PlSk.dat';dat.write_bytes(data)
 ninja=(build/'build.ninja').read_text();start=ninja.index('build native_source_assets_test.exe:');end=ninja.index('\n\n',start);block=ninja[start:end]
 libs=re.search(r'^  LINK_LIBRARIES = (.+)$',block,re.M).group(1).split()
 # Use the target's existing exact link dependencies, never invoking Ninja or
 # compiling/rewriting production libraries. Only this private probe is built.
 paths=[build/x for x in libs if x.endswith('.lib') and not x.startswith('-')]
 inputs=[compiler,HERE/'probe.c',HERE/'run.py',build/'build.ninja']+paths+list((ROOT/'src').rglob('*.h'))
 sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();hashes={str(path):sha(path) for path in inputs if path.exists()}
 command=[str(compiler),'-std=gnu11','-O2','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt','-fuse-ld=lld','-Wl,/opt:ref','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),str(HERE/'probe.c')]+libs+['-o',str(out/'probe.exe')]
 commands=[]
 for stage,cmd,cwd in [('compile',command,build),('probe',[str(out/'probe.exe'),str(dat)],out)]:
  if stage=='probe':
   for dll in build.glob('*.dll'):shutil.copy2(dll,out/dll.name)
  r=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-8000:])
 if any(sha(Path(path))!=value for path,value in hashes.items()):raise RuntimeError('library/header inputs changed during probe')
 receipt={'status':'PASS','results':[json.loads(x) for x in r.stdout.splitlines() if x.startswith('{')],'dat_sha256':hashlib.sha256(data).hexdigest(),'input_sha256':hashes,'commands':commands,'scope':'Actual retail Sheik DAT, existing owned character importer and named-root import/cache identity; no constructors, model instances, renderer/game or global build'}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['results']))
if __name__=='__main__':main()
