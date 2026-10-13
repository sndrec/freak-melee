"""Compile actual copied definitions and frozen import units; console only."""
import argparse, hashlib, json, re, shutil, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p = argparse.ArgumentParser()
    for name in ('root', 'baseline', 'iso', 'output'):
        p.add_argument('--' + name, required=True, type=Path)
    a = p.parse_args()
    root, base, out = a.root.resolve(), a.baseline.resolve(), a.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    sdk, commands = base / 'sdk', []
    def run(cmd, log, cwd=root):
        cmd = list(map(str, cmd));commands.append(cmd)
        result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
        (out / log).write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode:
            raise RuntimeError(str(out / log) + '\n' + (result.stdout + result.stderr)[-6500:])
        return result.stdout
    run([sys.executable, HERE / 'fixture.py', '--root', root, '--baseline', base,
         '--iso', a.iso.resolve(), '--output', out], 'prepare.log')
    overlay = out / 'overlay'
    for name in ('Popo', 'Nana'):
        directory = 'src/melee/ft/kinds/ft' + name
        (overlay / directory).mkdir(parents=True)
        for header in (sdk / directory).glob('*.h'):
            shutil.copyfile(header, overlay / directory / header.name)
        for suffix in ('.c', '.h'):
            file = 'ft' + name.lower() + '_clone' + suffix
            shutil.copyfile(root / directory / file, overlay / directory / file)
    sources = [HERE / 'test.c'] + [overlay / ('src/melee/ft/kinds/ft%s/ft%s_clone.c' % (n,n.lower()))
                                 for n in ('Popo','Nana')]
    imports = ['src/melee/ft/ftcharacter_assets.c', 'src/melee/ft/ftcostume_assets.c',
               'src/melee/ft/ftanimation_bank.c']
    sources += [sdk / rel for rel in imports]
    build = root / 'build/native'; ninja = (build / 'build.ninja').read_text()
    at = ninja.index('build native_fighter_character_assets_test.exe:')
    libraries = re.search(r'(?m)^  LINK_LIBRARIES = (.+)$', ninja[at:])[1].split()
    libraries = [str((build / x).resolve()) if not x.startswith('-') else x for x in libraries]
    library_hashes = {x:sha(Path(x)) for x in libraries if not x.startswith('-')}
    flags = ['-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT',
             '-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(overlay/'src'),
             '-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out)]
    lines = ['@echo off', 'call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul']
    objects = []
    for i,source in enumerate(sources):
        obj = out / ('unit%d.obj' % i);objects.append(obj)
        lines += [subprocess.list2cmdline(list(map(str,[base/'compiler/clang.exe',*flags,'-c',source,'-o',obj]))),
                  'if errorlevel 1 exit /b %errorlevel%']
    lines += [subprocess.list2cmdline(list(map(str,[base/'compiler/clang.exe',*objects,*libraries,
               '-fuse-ld=lld-link','-Wl,/opt:ref','-o',out/'test.exe'])))]
    batch = out / 'compile.cmd';batch.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    run(['cmd','/c',batch], 'compile.log')
    result = run([out/'test.exe',out], 'run.log', base/'bin')
    assert 'PASS 2 copied bases' in result
    for path,value in library_hashes.items():
        assert sha(Path(path))==value,'Linked library changed during console proof'
    files = set(sources) | set(p for p in HERE.iterdir() if p.is_file())
    files.update(p for p in (base/'compiler').rglob('*') if p.is_file())
    files.update(p for p in sdk.rglob('*') if p.is_file())
    files.update(root / ('src/melee/ft/kinds/ft%s/ft%s_clone%s' % (n,n.lower(),s))
                 for n in ('Popo','Nana') for s in ('.c','.h'))
    files.update((base/'freeze.json',root/'tools/fighter_animation_io.py',build/'build.ninja'))
    receipt = {'status':'PASS copied-base metadata and retail imports only','result':result.strip(),
               'commands':commands,'inputs':{str(p):sha(p) for p in sorted(files)},
               'support_libraries':library_hashes,'retail':json.loads((out/'inputs.json').read_text()),
               'boundaries':['Complete production import units compiled from frozen SDK; copied bases are explicit new overlays.',
                             'Complete frozen stock motion-table initializers; inert callback identities abort if invoked.',
                             'Support libraries are pinned link dependencies, not a rebuilt game or runtime activation.',
                             'No normal clone-stock registration, independent behavior, animation materialization or paired gameplay claim.']}
    receipt['outputs']={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file()}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(result.strip());print(out/'result.json')

if __name__=='__main__':
    main()
