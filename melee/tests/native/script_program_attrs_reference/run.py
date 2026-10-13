"""Test-only exporter program-parameter expressions; never an ordinary loader."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
 p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('fresh A: archive required')
 out.mkdir(parents=True)
 manifest=ROOT/'CMakeLists.txt';group=re.search(r'set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+(.*?)\)',manifest.read_text(),re.S)
 if not group:raise RuntimeError('reference source group absent')
 units=[ROOT/f for f in group.group(1).split()]+[ROOT/'src/melee/ft/native_fighter_parameters.c',ROOT/'src/melee/ft/native_fighter_parameter_import.c',ROOT/'src/melee/ft/native_fighter_article_bindings.c',ROOT/'src/sysdolphin/baselib/dat.c',HERE/'test.c']
 inputs=units+[HERE/'run.py',HERE/'README.md',a.compiler.resolve()]+list((ROOT/'tools').glob('*.h'))+list((ROOT/'src').rglob('*.h'))
 hashes={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in inputs}
 command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-sign-compare','-Wno-logical-op-parentheses','-Wno-unused-variable','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'tools'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,units),'-o',str(out/'test.exe')];commands=[]
 for stage,cmd in [('compile',command),('run',[str(out/'test.exe')])]:
  result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr);commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
  if result.returncode:raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-8000:])
 changed=[f for f,h in hashes.items() if hashlib.sha256(Path(f).read_bytes()).hexdigest()!=h]
 if changed:raise RuntimeError('inputs changed: '+','.join(changed))
 receipt={'status':'PASS','result':json.loads(result.stdout),'input_sha256':hashes,'commands':commands,'scope':['actual Article/Fighter reference expression/type parsers and complete GenerateC','I32/F32/Bool explicit root parameter emission matches module script_parameters array','unknown/call/raw-offset fallback rejected; Article local attribute typing preserved','actual parameter importer +DAT codec preserves explicit root reads and qualified local values','full127character parameter-name width'],'limits':['test-only C oracle; ordinary VM/loader implementation owned separately by root','no native runtime or full Game & Watch acceptance claim']};(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
