"""Prepare an unregistered Peach composition; no installed package or game."""
import argparse,hashlib,json,re,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('root','baseline','output'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--prepare-only',action='store_true');p.add_argument('--project',type=Path,help='Existing real registered Peach clone; absence explicitly leaves normal pipeline pending')
    a=p.parse_args();root=a.root.resolve();base=a.baseline.resolve();out=a.output.resolve();sdk=base/'sdk'
    if out.exists():raise ValueError('Use a fresh output directory')
    out.mkdir(parents=True);commands=[]
    def run(cmd,name):
        cmd=[str(x) for x in cmd];commands.append(cmd)
        r=subprocess.run(cmd,cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        (out/name).write_text(r.stdout,encoding='utf-8')
        if r.returncode:raise RuntimeError(r.stdout)
        return r.stdout
    clang=base/'compiler/clang.exe';tool=base/'bin/native_content_tool.exe'
    vcvars=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
    def compile(args,name):
        batch=out/(name+'.cmd')
        batch.write_text('@echo off\ncall "'+str(vcvars)+'" >nul\n'+subprocess.list2cmdline([str(x) for x in args])+'\n')
        run(['cmd','/c',batch],name+'.log')
    common=[clang,'-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include')]
    dat=root/'.local/peach-turnip-composition30/retail-peach.dat'
    if a.project:
        project=a.project.resolve()
        run([tool,'import-parameters',project,HERE/'candidate.fms',out/'owned.fms'],'import.log')
        import shutil
        shutil.copyfile(project/'fighter_parameters.fmp',out/'fighter_parameters.fmp')
    else:
        units=['src/melee/ft/native_fighter_parameter_import.c','src/melee/ft/native_fighter_parameters.c','src/melee/ft/native_fighter_article_bindings.c','src/sysdolphin/baselib/dat.c']
        compile(common+[HERE/'import.c',*[sdk/u for u in units],'-o',out/'import.exe'],'compile_import')
        run([out/'import.exe',dat,HERE/'candidate.fms',out/'owned.fms',out/'fighter_parameters.fmp'],'import.log')
    run([tool,'script-c',out/'owned.fms','peach_pluck_smash',out/'generated.inc'],'generate.log')
    compile(common+['-shared','-x','c',out/'generated.inc','-o',out/'module.dll'],'compile_module')
    if a.prepare_only:
        print('Prepared import/generated module:',out);return
    run([sys.executable,HERE/'fixture.py','--root',sdk,'--output',out,'--generated',out/'generated.inc','--parameters',out/'fighter_parameters.fmp','--dat',dat],'extract.log')
    units=['src/melee/ft/native_callback_lifetime.c','src/melee/it/native_item_owner.c']
    compile(common+['-I'+str(out),HERE/'test.c',*[sdk/u for u in units],'-o',out/'test.exe'],'compile_test')
    result=run([out/'test.exe'],'run.log')
    counts=re.search(r'(\d+) original/generated comparisons, (\d+) checks',result)
    if not counts:raise ValueError('Missing fixture result')
    inputs={path for path in sdk.rglob('*') if path.is_file()}
    inputs.update(path for path in HERE.iterdir() if path.is_file())
    inputs.update([dat,base/'freeze.json',tool,clang])
    components=json.loads((HERE/'components.json').read_text())
    for path,expected in components['source_inputs'].items():
        actual=hashlib.sha256((root/path).read_bytes()).hexdigest()
        if actual!=expected:raise ValueError('Accepted component changed: '+path)
        inputs.add(root/path)
    receipt={'status':'PASS unregistered source composition; normal clone/import pending' if not a.project else 'PASS unregistered normal-import source composition','snapshot':base.name,'normal_project_pipeline':bool(a.project),'comparisons':int(counts[1]),'checks':int(counts[2]),'authored_safety_cases':int(re.search(r'(\d+) authored safety cases',result)[1]),'commands':commands,'census':json.loads((out/'census.json').read_text()),'limits':['Frozen44 raw-DAT import mode has no registered Peach base. No install, package activation, gameplay or independent declaration is tested.','Float, Side-B, Toad and Parasol travel remain pending; this slice authors 349..353 and Explode.','Accepted composition44 constructor/cleanup/Turnip/setup evidence is reused by immutable input hash; new phases and Explode are compared here.','Animation, collision probes and allocation are controlled boundaries; actual source bodies and generated callbacks execute directly.'],'inputs':{str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(inputs)}}
    archive=out/'source_snapshot';archive.mkdir()
    import shutil
    for path in HERE.iterdir():
        if path.is_file():shutil.copyfile(path,archive/path.name)
    receipt['archived_namespace_inputs']={str(path):str(archive/path.name) for path in HERE.iterdir() if path.is_file()}
    receipt['outputs']={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(out.iterdir()) if path.is_file()}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(result.strip());print('Receipt:',out/'result.json')
if __name__=='__main__':main()
