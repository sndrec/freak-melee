"""Scoped retail asset acceptance, using built actual runtime import units."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser()
    for k in ('root','baseline','output','iso'):p.add_argument('--'+k,type=Path,required=True)
    a=p.parse_args();root=a.root.resolve();base=a.baseline.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);commands=[]
    def run(cmd,name,cwd=root):
        cmd=list(map(str,cmd));commands.append(cmd);r=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True)
        (out/name).write_text(r.stdout+r.stderr,encoding='utf-8')
        if r.returncode:raise RuntimeError(str(out/name)+'\n'+(r.stdout+r.stderr)[-7000:])
        return r.stdout
    run([sys.executable,HERE/'fixture.py','--root',root,'--output',out],'extract.log')
    run([sys.executable,HERE/'retail_fixture.py','--root',root,'--iso',a.iso.resolve(),'--output',out],'retail-extract.log')
    build=root/'build/native';ninja=(build/'build.ninja').read_text()
    target=ninja.index('build native_fighter_character_assets_test.exe:')
    libraries=re.search(r'(?m)^  LINK_LIBRARIES = (.+)$',ninja[target:])[1].split()
    libraries=[str((build/x).resolve()) if not x.startswith('-') else x for x in libraries]
    hashes={x:hashlib.sha256(Path(x).read_bytes()).hexdigest() for x in libraries if not x.startswith('-')}
    sources=[HERE/'retail_test.c',root/'src/melee/ft/native_fighter_pose.c',root/'src/melee/ft/native_fighter_pose_path.c',root/'src/melee/ft/native_callback_lifetime.c',root/'src/melee/ft/ftparts_table.c']
    flags=['-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(root/'src'),'-I'+str(root/'extern/dolphin/include'),'-I'+str(out)]
    lines=['@echo off','call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul']
    objects=[]
    for i,source in enumerate(sources):
        obj=out/('unit'+str(i)+'.obj');objects.append(obj)
        lines.extend([subprocess.list2cmdline(list(map(str,[base/'compiler/clang.exe',*flags,'-c',source,'-o',obj]))),'if errorlevel 1 exit /b %errorlevel%'])
    lines.append(subprocess.list2cmdline(list(map(str,[base/'compiler/clang.exe',*objects,*libraries,'-fuse-ld=lld-link','-Wl,/opt:ref','-o',out/'retail.exe']))))
    batch=out/'compile.cmd';batch.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    run(['cmd','/c',batch],'compile.log')
    # Console process only. DLL search follows the existing actual asset target.
    result=run([out/'retail.exe',out],'run.log',build)
    for name,digest in hashes.items():assert hashlib.sha256(Path(name).read_bytes()).hexdigest()==digest,'library changed during proof'
    inputs=set(sources)|{HERE/'fixture.py',HERE/'retail_fixture.py',HERE/'run_retail.py',build/'build.ninja'}
    inputs.update(p for p in (root/'src').rglob('*.h'));inputs.update(p for p in (root/'extern/dolphin/include').rglob('*.h'))
    for data in ('sources.json','retail_inputs.json'):
        manifest=json.loads((out/data).read_text())
        for name in (manifest if data=='sources.json' else manifest['sources']):inputs.add(root/name)
    inputs.update(p for p in (base/'compiler').rglob('*') if p.is_file())
    for name in ('src/melee/ft/ftcharacter_assets.c','src/melee/ft/ftcostume_assets.c','src/melee/ft/ftanimation_bank.c'):inputs.add(root/name)
    receipt={'status':'PASS retail pose asset topology','result':result.strip(),'commands':commands,'inputs':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(inputs)},'linked_libraries':hashes,'limits':['Complete production imports, owned pose resolvers/validator, real part initialization/setup.','JObjs materialized from exact retail descriptor topology/flags/SRT; material objects and allocation controlled.','No animation evaluation/interpolation math, generated Chain callbacks or gameplay.']}
    receipt['outputs']={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.rglob('*')) if p.is_file()}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(result.strip());print(out/'result.json')
if __name__=='__main__':main()
