"""Actual resource frontend/Item preparation and controlled native-marshalling VM cohort."""
import argparse, hashlib, json, re, shutil, subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
 build=ROOT/'build/native';ninja=(build/'build.ninja').read_text();begin=ninja.index('build native_source_assets_test.exe:');block=ninja[begin:ninja.index('\n\n',begin)];libraries=re.search(r'^  LINK_LIBRARIES = (.+)$',block,re.M).group(1).split()
 out.mkdir(parents=True);snapshot=out/'snapshot';sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();live={}
 def copy(path,destination):
  destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,destination);live[str(path.resolve())]=sha(destination)
 for folder,name in [(ROOT/'src','src'),(ROOT/'extern/dolphin/include','dolphin'),(ROOT.parent/'aurora/include','aurora')]:
  for path in folder.rglob('*'):
   if path.is_file() and path.suffix in ['.h','.hpp','.c']:copy(path,snapshot/name/path.relative_to(folder))
 for path in [HERE/'test.c',HERE/'run.py',HERE/'README.md',ROOT/'tests/native/script_bindings_unavailable_item_resources.c']:
  copy(path,snapshot/'fixture'/path.name)
 copy(build/'build.ninja',snapshot/'build.ninja');linked=[]
 for name in libraries:
  path=build/name
  if path.is_file():destination=snapshot/'lib'/path.relative_to(build);copy(path,destination);linked.append(str(destination))
  else:linked.append(name)
 for dll in build.glob('*.dll'):copy(dll,out/dll.name)
 compiler=a.compiler.resolve();inputs={str(path):sha(path) for path in out.rglob('*') if path.is_file()};inputs[str(compiler)]=sha(compiler)
 command=[str(compiler),'-std=gnu11','-O2','-g','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt','-fuse-ld=lld','-Wl,/opt:ref','-I'+str(snapshot/'src'),'-I'+str(snapshot/'dolphin'),'-I'+str(snapshot/'aurora'),str(snapshot/'fixture/test.c'),str(snapshot/'fixture/script_bindings_unavailable_item_resources.c')]+linked+['-o',str(out/'test.exe')]
 commands=[];summary=None
 for stage,cmd,cwd in [('compile',command,build),('run',[str(out/'test.exe')],out)]:
  r=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if r.returncode:break
  if stage=='run':summary=json.loads(r.stdout)
 unchanged=all(sha(Path(path))==value for path,value in inputs.items());status='PASS' if r.returncode==0 and unchanged else 'FAIL'
 receipt={'status':status,'summary':summary,'commands':commands,'input_sha256':inputs,'inputs_unchanged':unchanged,'source_snapshot_sha256':live,'live_sources_match_snapshot':all(Path(path).exists() and sha(Path(path))==value for path,value in live.items()),'scope':['actual Source_Parse/Program_Index/Compile/Bytecode, storage, and ItemPrograms_Prepare from rebuilt native libraries','resource descriptor, exact owned roots, prepared decoded joint channels and matrix declaration constants, aggregate selector rebind and transaction failures','actual VM dynamic index single evaluation before effectful RHS, full-width integral selectors, vector output-slot publication and source-retirement continuation'],'controlled':['synthetic owned Article/model descriptors and asset/facts bindings (no DAT importer claim)','recording VM host accepts only the explicitly asserted frontend marshalling operations; it does not implement native resource behavior','real resource runtime absent leaf rejects Read/Write/Invoke with unchanged values'],'limits':['no resource scene/ItemLink allocation, actual collision/model/effect operation, live Item actor or native resource lifetime proof','no graphics/game/whole-match acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'status':status,'summary':summary,'receipt':str(out/'receipt.json')}));print((r.stdout+r.stderr)[-6000:]);raise SystemExit(r.returncode or (not unchanged))
if __name__=='__main__':main()
