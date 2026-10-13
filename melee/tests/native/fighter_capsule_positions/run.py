"""Scoped console compiler/generated-module checks; no CMake or gameplay."""
import argparse, hashlib, json, re, subprocess, shutil
from pathlib import Path
HERE = Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();root=a.root.resolve();base=a.baseline.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    sdk=base/'sdk';clang=base/'compiler/clang.exe';commands=[]
    def run(command,name):
        command=list(map(str,command));commands.append(command)
        r=subprocess.run(command,cwd=root,capture_output=True,text=True);(out/name).write_text(r.stdout+r.stderr,encoding='utf-8')
        if r.returncode: raise RuntimeError(str(out/name)+'\n'+(r.stdout+r.stderr)[-8000:])
        return r.stdout
    def compile(command,name):
        batch=out/(name+'.cmd');batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,command)))+'\n',encoding='utf-8');run(['cmd','/c',batch],name+'.log')
    flags=[clang,'-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out)]
    cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8');body=re.search(r'set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+(.*?)\)',cmake,re.S)[1]
    sources=[root/path for path in body.split()]+[sdk/'src/melee/ft/native_fighter_parameters.c']
    compile(flags+[HERE/'compiler_test.c']+sources+['-o',out/'compiler_test.exe'],'compile_compiler')
    run([out/'compiler_test.exe',HERE/'fixture.fms',out/'generated.inc',out/'generated_state.inc'],'compiler.log')
    sourcepath=sdk/'src/melee/ft/ftcoll.c'
    run(['python',HERE/'fixture.py',sourcepath,out],'extract.log')
    compile(flags+[HERE/'module_test.c','-o',out/'module_test.exe'],'compile_module')
    result=run([out/'module_test.exe'],'module.log')
    compile(flags+['-DCAPSULE_STATE_ROUTE=1',HERE/'module_test.c','-o',out/'state_module_test.exe'],'compile_state_module')
    state_result=run([out/'state_module_test.exe'],'state_module.log')
    inputs=set(sources+[HERE/'compiler_test.c',HERE/'module_test.c',HERE/'fixture.fms',HERE/'run.py',HERE/'fixture.py',sourcepath,base/'freeze.json'])
    inputs.update(p for p in sdk.rglob('*') if p.is_file());inputs.update(p for p in (base/'compiler').rglob('*') if p.is_file())
    receipt={'status':'PASS generic capsule coordinates compiler/generated runtime','baseline':str(base),'compiler_sources':'LIVE scoped compiler; immutable SDK fields','commands':commands,'result':result.strip(),'state_route_result':state_result.strip(),'inputs':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(inputs)},'outputs':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*')) if p.is_file()},'limits':['Actual generated module executes direct coordinate writes/reads; no installed package or gameplay.','Complete ftColl_AD18/B8A8 execute with controlled joint position boundary; not whole collision traversal or Sheik Chain.']}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(result.strip());print(out/'result.json')
if __name__=='__main__':main()
