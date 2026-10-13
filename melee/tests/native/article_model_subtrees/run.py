"""Scoped live-source compiler/modules and complete model subtree routing bodies."""
from pathlib import Path
import argparse, hashlib, json, re, subprocess
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def body(source, name):
    match = re.search(r'(?m)^(?:static\s+)?[A-Za-z_][\w\s*]*\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', source)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[match.start():end]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args(); out = a.output.resolve(); clang = a.compiler.resolve()
    if out.exists() or out.drive.upper() != 'A:': p.error('Fresh A: output required')
    out.mkdir(parents=True)
    cmake = ROOT/'CMakeLists.txt'
    source_list = re.search(r'set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+(.*?)\)', cmake.read_text(encoding='utf-8'), re.S).group(1)
    units = [ROOT/name for name in source_list.split()] + [ROOT/'src/melee/ft/native_fighter_parameters.c', ROOT/'tests/native/article_animation_components_compiler_test.c']
    selected = {'src/melee/lb/native_model_joint.c':['nativeModelJoint_Resolve'],
        'src/melee/it/native_article_model.c':['model_root','nativeArticleModel_AddAnimation','nativeArticleModel_RequestAnimation','nativeArticleModel_AdvanceAnimation','nativeArticleModel_RemoveAnimation']}
    pieces = []
    for relative, names in selected.items():
        source = (ROOT/relative).read_text(encoding='utf-8')
        pieces += ['// '+relative+' '+name+'\n'+body(source, name) for name in names]
    (out/'actual.h').write_text('\n\n'.join(pieces)+'\n', encoding='utf-8')
    paths = units + [ROOT/name for name in selected] + [cmake, HERE/'test.c', HERE/'run.py', ROOT/'tests/native/article_animation_components_module_test.c', clang]
    paths += list((ROOT/'tools').glob('*.h')) + list((ROOT/'src').rglob('*.h')) + list((ROOT/'extern/dolphin/include').rglob('*.h'))
    inputs = {str(path):sha(path) for path in paths}
    common = [str(clang), '-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
    commands, results = [], {}
    def run(label, command):
        command = list(map(str, command)); commands.append(command)
        if label.startswith('compile'):
            batch = out/(label+'.cmd')
            batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\n'+subprocess.list2cmdline(command)+'\n', encoding='utf-8')
            command = ['cmd','/d','/c',str(batch)]
        r = subprocess.run(command,cwd=ROOT,text=True,encoding='utf-8',capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW,timeout=180)
        (out/(label+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8')
        if r.returncode: raise RuntimeError(label+' failed:\n'+r.stdout+r.stderr)
        results[label] = r.stdout.strip()
    exe = out/'compiler.exe'
    run('compile-compiler',common+units+['-o',exe])
    run('generate',[exe,out/'article_components_action_fixture.h',out/'article_components_callback_fixture.h'])
    for callbacks in (0,1):
        exe = out/f'module{callbacks}.exe'
        run(f'compile-module{callbacks}',common+[f'-DARTICLE_COMPONENTS_CALLBACKS={callbacks}',ROOT/'tests/native/article_animation_components_module_test.c','-o',exe])
        run(f'module{callbacks}',[exe])
    exe = out/'engine.exe'
    run('compile-engine',common+[HERE/'test.c','-o',exe]); run('engine',[exe])
    for path, expected in inputs.items(): assert sha(Path(path)) == expected, path
    receipt = {'status':'PASS','results':results,'inputs':inputs,'commands':commands,'extracted_sha256':sha(out/'actual.h'),
        'boundaries':['Live exact source/header candidate, explicit immutable compiler binary; no frozen SDK claim.',
            'Both actual compiler routes and generated callbacks, six PAGE_NOACCESS retirement stops per route.',
            'Complete production model selection/path/Add/Request/Advance/Remove bodies; HSD traversal is a controlled observed sink.',
            'Real HSD subtree evaluation is additionally covered in native_model_runtime, run separately by root. No renderer/game/package acceptance.']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8'); print(json.dumps(results))

if __name__ == '__main__': main()
