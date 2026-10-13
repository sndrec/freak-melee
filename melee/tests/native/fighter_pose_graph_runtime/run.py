"""Scoped actual-source pose runtime console proof; no game/whole build."""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser()
    for k in ('root','baseline','output'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();root=a.root.resolve();base=a.baseline.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);commands=[]
    def run(cmd,name):
        cmd=list(map(str,cmd));commands.append(cmd);r=subprocess.run(cmd,cwd=root,capture_output=True,text=True)
        (out/name).write_text(r.stdout+r.stderr,encoding='utf-8')
        if r.returncode:raise RuntimeError(str(out/name)+'\n'+(r.stdout+r.stderr)[-7000:])
        return r.stdout
    run([sys.executable,HERE/'fixture.py','--root',root,'--output',out],'extract.log')
    sources=[HERE/'test.c',root/'src/melee/ft/native_fighter_pose.c',root/'src/melee/ft/native_fighter_pose_path.c',root/'src/melee/ft/native_callback_lifetime.c',root/'src/melee/ft/ftparts_table.c']
    cmd=[base/'compiler/clang.exe','-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(root/'src'),'-I'+str(root/'extern/dolphin/include'),'-I'+str(out)]+sources+['-o',out/'test.exe']
    batch=out/'compile.cmd';batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,cmd)))+'\n',encoding='utf-8')
    run(['cmd','/c',batch],'compile.log');result=run([out/'test.exe'],'run.log')
    inputs=set(sources)|{HERE/'fixture.py',HERE/'run.py'};inputs.update(p for p in (root/'src').rglob('*.h'));inputs.update(p for p in (root/'extern/dolphin/include').rglob('*.h'))
    for rel in json.loads((out/'sources.json').read_text()):inputs.add(root/rel)
    inputs.update(p for p in (base/'compiler').rglob('*') if p.is_file())
    receipt={'status':'PASS owned pose runtime source boundary','toolchain':str(base),'result':result.strip(),'commands':commands,'inputs':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(inputs)},'outputs':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*')) if p.is_file()},'limits':['Live changed engine+headers compiled by frozen62 compiler; no new SDK frozen yet.','Actual resolver and eight actual shared traversal bodies and actual descriptor iterator; lower SRT/track installation controlled.','Synthetic topology validation safety is separate from real retail graph/costume acceptance; no generated Chain policy or gameplay.']}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(result.strip());print(out/'result.json')
if __name__=='__main__':main()
