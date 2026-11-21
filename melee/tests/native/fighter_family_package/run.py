"""Canonical source family packet codec; exact actual data validation bodies."""
import argparse,hashlib,json,subprocess,importlib.util,shutil,sys
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('source_packet_extract',HERE/'extract.py');extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--build',type=Path,default=ROOT/'build/native');a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: output required')
 hashes=extractor.extract(ROOT,out);compiler=a.compiler.resolve();compiler=compiler/'clang.exe' if compiler.is_dir() else compiler
 units=[ROOT/'src/melee/gm/native_content_store.c',ROOT/'src/melee/ft/native_file_publish.c',ROOT/'src/melee/gm/native_fighter_package.c',ROOT/'src/melee/ft/native_source_asset_record.c',ROOT/'src/melee/ft/native_source_resources.c',ROOT/'src/melee/ft/native_fighter_sound_policy.c',HERE/'test.c']
 zlib_build=a.build.resolve()/'_deps/zlib-build';zlib_lib=zlib_build/'zlib.lib';zlib_dll=zlib_build/'zlib1.dll'
 shutil.copyfile(zlib_dll,out/zlib_dll.name)
 inputs=units+[compiler,HERE/'extract.py',HERE/'run.py',*zlib_build.glob('*.h'),zlib_lib,zlib_dll]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'));sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();hashes.update({str(p):sha(p) for p in inputs});commands=[]
 command=[str(compiler),'-std=gnu11','-O2','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-Isrc','-Iextern/dolphin/include','-I'+str(out),'-I'+str(zlib_build)]+list(map(str,units))+[str(zlib_lib),'-lbcrypt','-o',str(out/'test.exe')]
 for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+' failed:\n'+(result.stdout+result.stderr)[-7000:])
 if any(sha(Path(p))!=h for p,h in hashes.items()):raise RuntimeError('input changed')
 receipt={'status':'PASS','summary':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'boundaries':['actual source packet/record codec + exact production pure facts/layout/move/path validation','valid synthetic numeric facts; DAT bytes controlled opaque transport payloads'],'limits':['no DAT graph/import/FMS runtime validation in codec fixture','actual source preparation tested separately; no old-format bridge or paired gameplay acceptance']}
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':receipt['summary']}))
if __name__=='__main__':main()
