"""Frozen importer plus candidate DAT-copy unit; console only, outputs on archive drive."""
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser()
    for name in ('root','baseline','iso','output'):
        p.add_argument('--'+name,required=True,type=Path)
    a=p.parse_args();root,base,out=a.root.resolve(),a.baseline.resolve(),a.output.resolve()
    out.mkdir(parents=True,exist_ok=False);sdk=base/'sdk';commands=[]
    sys.path.insert(0,str(root/'tools'))
    from fighter_animation_io import disc_file
    retail={}
    for name in ('PlPp.dat','PlNn.dat'):
        data=disc_file(a.iso.resolve(),name);(out/name).write_bytes(data)
        retail[name]=hashlib.sha256(data).hexdigest()
    staged=HERE/'native_fighter_dat_copy.c'
    copier=staged if staged.exists() else root/'src/melee/ft/native_fighter_dat_copy.c'
    header=copier.with_suffix('.h')
    sources=[HERE/'test.c',copier]+[sdk/('src/melee/ft/'+name+'.c') for name in
                                  ('ftcharacter_assets','ftcostume_assets','ftanimation_bank')]
    build=root/'build/native';ninja=(build/'build.ninja').read_text(encoding='utf-8')
    at=ninja.index('build native_fighter_character_assets_test.exe:')
    libraries=re.search(r'(?m)^  LINK_LIBRARIES = (.+)$',ninja[at:])[1].split()
    libraries=[str((build/x).resolve()) if not x.startswith('-') else x for x in libraries]
    library_hashes={x:sha(Path(x)) for x in libraries if not x.startswith('-')}
    inputs=set(sources)|{header,root/'tools/fighter_animation_io.py',base/'freeze.json',build/'build.ninja'}
    inputs.update(x for x in HERE.iterdir() if x.is_file())
    inputs.update(x for x in (base/'compiler').rglob('*') if x.is_file())
    inputs.update(x for x in sdk.rglob('*') if x.is_file())
    hashes={str(x):sha(x) for x in sorted(inputs)}
    def run(cmd,log,cwd=root):
        cmd=list(map(str,cmd));commands.append(cmd)
        done=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True)
        (out/log).write_text(done.stdout+done.stderr,encoding='utf-8')
        if done.returncode:raise RuntimeError(str(out/log)+'\n'+(done.stdout+done.stderr)[-7000:])
        return done.stdout
    flags=['-std=gnu11','-O2','-Wall','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT',
           '-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(copier.parent),
           '-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include')]
    lines=['@echo off','call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul'];objects=[]
    for i,source in enumerate(sources):
        obj=out/('unit%d.obj'%i);objects.append(obj)
        lines += [subprocess.list2cmdline(list(map(str,[base/'compiler/clang.exe',*flags,'-c',source,'-o',obj]))),'if errorlevel 1 exit /b %errorlevel%']
    lines += [subprocess.list2cmdline(list(map(str,[base/'compiler/clang.exe',*objects,*libraries,'-fuse-ld=lld-link','-Wl,/opt:ref','-o',out/'test.exe'])))]
    batch=out/'compile.cmd';batch.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    run(['cmd','/c',batch],'compile.log')
    result=run([out/'test.exe',out],'run.log',base/'bin');assert 'PASS DAT pointer transplant:' in result
    for path,value in hashes.items():assert sha(Path(path))==value,'Input changed: '+path
    for path,value in library_hashes.items():assert sha(Path(path))==value,'Linked dependency changed: '+path
    receipt={'status':'PASS generic DAT pointer transplant and actual retail article imports',
             'result':result.strip(),'commands':commands,'inputs':hashes,'support_libraries':library_hashes,
             'retail_archives':retail,'boundaries':[
                 'Actual complete frozen71 character/costume/animation importer units; no substitute import algorithm.',
                 'Actual HSD_DatOpen/Resolve and shared graph/item decoders linked from hash-pinned support libraries.',
                 'Synthetic fixtures exercise serialized relocation/external semantics and failed-output transactionality.',
                 'Retail gate appends whole Popo data into Nana and imports the entire three-article table; old Nana graph placeholders remain external but unreachable from that table.',
                 'No runtime donor lookup, public source-root export, animation-bank materialization, family registration, independent activation or gameplay.']}
    receipt['outputs']={str(x.relative_to(out)):sha(x) for x in out.rglob('*') if x.is_file()}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(result.strip());print(out/'result.json')
if __name__=='__main__':main()
