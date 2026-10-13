"""Run bounded actual-source restart continuation comparisons, no game."""
import argparse,hashlib,json,subprocess,sys,shutil
from pathlib import Path
HERE=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('root','original-sdk','compiler','output'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--candidate-sdk',type=Path);args=parser.parse_args();root=args.root.resolve();old=args.original_sdk.resolve();sdk=(args.candidate_sdk or root).resolve();compiler=args.compiler.resolve();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False);commands=[]
    def run(command,name):
        command=list(map(str,command));commands.append(command);r=subprocess.run(command,cwd=root,text=True,capture_output=True);(out/name).write_text(r.stdout+r.stderr)
        if r.returncode:raise RuntimeError(str(out/name)+'\n'+(r.stdout+r.stderr)[-5000:])
        return r.stdout
    run([sys.executable,HERE/'fixture.py','--original-sdk',old,'--candidate-sdk',sdk,'--output',out],'extract.log')
    command=[compiler,'-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out),HERE/'test.c',sdk/'src/melee/ft/native_callback_lifetime.c','-o',out/'test.exe']
    batch=out/'compile.cmd';batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,command)))+'\n');run(['cmd','/c',batch],'compile.log');result=run([out/'test.exe'],'run.log')
    manifest=json.loads((out/'manifest.json').read_text());inputs={Path(v['path']) for v in manifest.values()};inputs.update(p for p in (sdk/'src').rglob('*.h'));inputs.update(p for p in (sdk/'extern/dolphin/include').rglob('*.h'));inputs.update(p for p in compiler.parent.rglob('*') if p.is_file());inputs.update(p for p in HERE.iterdir() if p.is_file());inputs.update([sdk/'src/melee/ft/native_callback_lifetime.c',HERE.parent/'normal_attack_contracts/source_fixture.py'])
    for p in (old.parent/'freeze.json',sdk.parent/'freeze.json'):
        if p.is_file():inputs.add(p)
    archive=out/'source_snapshot';archive.mkdir()
    for p in HERE.iterdir():
        if p.is_file():shutil.copyfile(p,archive/p.name)
    receipt={'status':'PASS bounded jab restart continuation','result':result.strip(),'commands':commands,'manifest':manifest,'inputs':{str(p):sha(p) for p in sorted(inputs)},'outputs':{str(p):sha(p) for p in out.rglob('*') if p.is_file()},'limits':['Actual doAttack12/doAttack13 decisions and continuation guards only.','Common jab/Attack12 normal/rapid entries, pickup and motion publication are explicit controls.','Healthy original58/live pairs separate from safety; original stale-pointer paths never executed.','Initial engine dispatch requires readable Fighter userdata; source-absent tests remove registration while retaining readable initial userdata.','No generated-script, gameplay, full normal-entry startup or arbitrary stale initial-pointer safety claim.']};(out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n');print(result.strip());print(out/'result.json')
if __name__=='__main__':main()
