"""Scoped actual sound source comparison and compiler/module metadata proof."""
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
def main():
    parser=argparse.ArgumentParser()
    for key in ('root','baseline','output'):parser.add_argument('--'+key,type=Path,required=True)
    args=parser.parse_args();root,base,out=args.root.resolve(),args.baseline.resolve(),args.output.resolve()
    out.mkdir(parents=True,exist_ok=False);commands=[]
    def run(command,name):
        command=list(map(str,command));commands.append(command)
        result=subprocess.run(command,cwd=root,text=True,capture_output=True)
        (out/name).write_text(result.stdout+result.stderr,encoding='utf-8')
        if result.returncode:raise RuntimeError(str(out/name)+'\n'+(result.stdout+result.stderr)[-12000:])
        return result.stdout.strip()
    run([sys.executable,HERE/'fixture.py','--root',root,'--baseline',base,'--output',out],'extract.log')
    cmake=(root/'CMakeLists.txt').read_text(encoding='utf-8')
    source_list=re.search(r'set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+(.*?)\)',cmake,re.S)[1]
    compiler_sources=[root/name for name in source_list.split()]
    inputs=set(compiler_sources)|{root/'src/melee/ft/native_fighter_parameters.c',HERE/'run.py',HERE/'fixture.py',HERE/'compiler.c',HERE/'module_test.c',HERE/'source_test.c'}
    inputs.update(Path(name) for name in json.loads((out/'sources.json').read_text(encoding='utf-8')))
    inputs.update(path for path in (root/'src').rglob('*.h'));inputs.update(path for path in (root/'tools').glob('*.h'))
    inputs.update(path for path in (root/'extern/dolphin/include').rglob('*.h'))
    inputs.update(path for path in (base/'compiler').rglob('*') if path.is_file())
    hashes={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(inputs)}
    def compile(sources,target,defines=()):
        command=[base/'compiler/clang.exe','-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(root/'src'),'-I'+str(root/'extern/dolphin/include'),'-I'+str(out),*defines,*sources,'-o',out/(target+'.exe')]
        batch=out/(target+'.cmd');batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,command)))+'\n',encoding='utf-8')
        run(['cmd','/c',batch],target+'-compile.log')
    compile([HERE/'source_test.c'],'source');results=[run([out/'source.exe'],'source-run.log')]
    compile([HERE/'compiler.c',*compiler_sources,root/'src/melee/ft/native_fighter_parameters.c'],'compiler')
    names=['decl_action','decl_state','inherit_action','inherit_state']
    results.append(run([out/'compiler.exe',*[out/(name+'.inc') for name in names]],'compiler-run.log'))
    for i,name in enumerate(names):
        compile([HERE/'module_test.c'],name,['-DSOUND_MODULE="'+name+'.inc"','-DSOUND_INHERIT='+str(int(i>=2))]);results.append(run([out/(name+'.exe')],name+'-run.log'))
    for path in inputs:
        if hashlib.sha256(path.read_bytes()).hexdigest()!=hashes[str(path)]:raise RuntimeError('Input changed during test: '+str(path))
    receipt={'status':'PASS owned sound channel source/compiler/module policy','baseline':str(base),'results':results,'commands':commands,'input_sha256':hashes,
        'output_sha256':{str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(out.rglob('*')) if path.is_file()},
        'limits':['Complete actual old/current sound dispatch/play/stop bodies; audio backend, mapped sound ID and deterministic random results controlled.',
                  'Actual registry lookup and mask resolver, controlled active-slot publication; no imported runtime package or audio mixing.',
                  'All128 authored masks have explicit filtering/RNG-preservation contract distinct from stock defaults; no game or desktop audio run.']}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print('\n'.join(results));print(out/'result.json')
if __name__=='__main__':main()
