"""Pure actual resource codec proof; hidden console only, no game/build."""
import argparse,hashlib,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]

def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 out.mkdir(parents=True);compiler=a.compiler.resolve();units=[ROOT/'src/melee/ft/native_source_resources.c',HERE/'test.c']
 inputs=units+[ROOT/'src/melee/ft/native_source_resources.h',HERE/'run.py',compiler]+list((ROOT/'examples/rook_workshop/items').glob('*.png'))
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();hashes={str(p):sha(p) for p in inputs};commands=[]
 header=out/'header.cpp';header.write_text('#include <melee/ft/native_source_resources.h>\nstatic_assert(NATIVE_SOURCE_RESOURCE_ENTRIES==64,"bounded directory");\nNativeSourceResources view{};\n')
 command=[str(compiler),'-std=gnu11','-O2','-Wall','-Wextra','-Werror','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src')]+list(map(str,units))+['-o',str(out/'test.exe')]
 header_command=[str(compiler),'-x','c++','-std=c++17','-fsyntax-only','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src'),str(header)]
 for stage,cmd in [('header-cpp',header_command),('compile',command),('fixture',[str(out/'test.exe'),str(ROOT)])]:
  r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:raise RuntimeError(stage+' failed:\n'+r.stdout+r.stderr)
 if any(sha(Path(p))!=v for p,v in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,'scope':'Actual canonical codec, 8 unchanged Rook PNG inputs, opaque binary resources; no allocation or file-backed pointer in production codec','limits':['PNG framing/CRC/IHDR proof, not pixel decoder success','no package integration, native Item blueprint/renderer or gameplay acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
