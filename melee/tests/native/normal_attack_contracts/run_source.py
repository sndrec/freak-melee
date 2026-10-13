"""Build/run source-only normal attack contracts with bundled clang."""
import argparse, hashlib, json, subprocess, sys, shutil
from pathlib import Path
HERE = Path(__file__).resolve().parent

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'original-sdk', 'compiler', 'output'): parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--candidate-sdk', type=Path)
    args = parser.parse_args(); root = args.root.resolve(); old = args.original_sdk.resolve(); sdk = (args.candidate_sdk or root).resolve(); compiler = args.compiler.resolve(); out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False); commands=[]
    def run(command, name):
        command=list(map(str,command));commands.append(command)
        result=subprocess.run(command,cwd=root,capture_output=True,text=True)
        (out/name).write_text(result.stdout+result.stderr)
        if result.returncode: raise RuntimeError(str(out/name)+'\n'+(result.stdout+result.stderr)[-7000:])
        return result.stdout
    run([sys.executable,HERE/'source_fixture.py','--original-sdk',old,'--candidate-sdk',sdk,'--output',out],'extract.log')
    arguments=[compiler,'-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out),HERE/'source_test.c',sdk/'src/melee/ft/native_callback_lifetime.c','-o',out/'test.exe']
    batch=out/'compile.cmd';batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,arguments)))+'\n')
    run(['cmd','/c',batch],'compile.log');result=run([out/'test.exe'],'run.log')
    manifest=json.loads((out/'source_manifest.json').read_text());inputs={Path(value['path']) for value in manifest.values()}
    inputs.update(p for p in (sdk/'src').rglob('*.h'));inputs.update(p for p in (sdk/'extern/dolphin/include').rglob('*.h'));inputs.update(p for p in compiler.parent.rglob('*') if p.is_file())
    inputs.update([HERE/'source_fixture.py',HERE/'source_test.c',HERE/'run_source.py',HERE/'README-source.md',sdk/'src/melee/ft/native_callback_lifetime.c'])
    for path in (old.parent/'freeze.json',sdk.parent/'freeze.json'):
        if path.is_file(): inputs.add(path)
    archive=out/'source_snapshot';archive.mkdir()
    for filename in ('source_fixture.py','source_test.c','run_source.py','README-source.md'):
        shutil.copyfile(HERE/filename,archive/filename)
    receipt={'status':'PASS actual original/current normal attack source contracts','result':result.strip(),'commands':commands,'source_manifest':manifest,
        'inputs':{str(p):sha(p) for p in sorted(inputs)},'outputs':{str(p):sha(p) for p in out.iterdir() if p.is_file()},
        'limits':['Console source proof, no gameplay or generated-script proof (separate root fixtures).','Allocation/pickup, motion/animation, private family entries and stick-angle/edge queries are controlled.','Healthy original/current pairs distinct from authored remapping/lifetime contracts; no stale original bodies executed.','No undefined union bytes, object addresses or struct padding compared.','Existing doAttack12/doAttack13 on_jab_restart continuation guards are absent in58 and outside claimed safety; separate follow-up pending.']}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n');print(result.strip());print(out/'result.json')

if __name__=='__main__':main()
