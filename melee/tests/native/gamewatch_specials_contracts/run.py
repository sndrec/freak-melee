"""Generated generic contracts and unchanged Oil Panic comparison; no game."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
sys.dont_write_bytecode = True
from compose import compose

HERE = Path(__file__).resolve().parent
FUNCTIONS = {
    'src/melee/ft/kinds/ftGameWatch/ftgamewatchspeciallw.c': [
        'ftGw_SpecialLw_UpdateBucketModel', 'ftGw_SpecialLw_AbsorbThink_DecideAction',
        'ftGw_SpecialLw_IASA'],
    'src/melee/it/kinds/itgamewatchpanic.c': ['itGameWatchPanic_Logic78_PickedUp'],
    'src/melee/ft/ftlib.c': ['ftLib_8008741C', 'ftLib_80087460'],
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def extract(source, name):
    match = re.search(r'(?m)^[A-Za-z_][^\n;{}]*\b' + re.escape(name) + r'\([^;]*?\)\s*\{', source)
    if not match:
        raise RuntimeError('Missing original: ' + name)
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def run(command, log):
    result = subprocess.run(command, capture_output=True, text=True)
    log.write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'Failed ({result.returncode}): {log}\n{result.stdout}{result.stderr}')
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if any(output == p or p in output.parents for p in [snapshot, HERE]):
        raise RuntimeError('Output must not modify immutable inputs.')
    for path, expected in json.loads((snapshot / 'freeze.json').read_text()).items():
        if sha(snapshot / path.replace('\\', '/')) != expected:
            raise RuntimeError('Snapshot changed: ' + path)
    output.mkdir(parents=True, exist_ok=True)
    sources, bodies = {}, []
    for path, names in FUNCTIONS.items():
        file = snapshot / 'sdk' / path
        sources[path] = sha(file)
        bodies.extend(extract(file.read_text(), name) for name in names)
    if sources != json.loads((HERE / 'original_inputs.json').read_text()):
        raise RuntimeError('Original inputs changed; review before accepting new hashes.')
    (output / 'original.h').write_text('\n'.join(bodies), encoding='utf-8')
    for name in ['probe.fms', 'compare.c']:
        shutil.copyfile(HERE / name, output / name)
    generated = output / 'behavior.generated.h'
    run([str(snapshot / 'bin/native_content_tool.exe'), 'script-c',
         str(output / 'probe.fms'), 'gamewatch_contract_probe', str(generated)], output / 'generate.log')
    composed = compose(output / 'composition')
    run([str(snapshot / 'bin/native_content_tool.exe'), 'script-c',
         str(composed), 'gamewatch_specials_draft', str(output / 'composition.generated.c')], output / 'composition.log')
    vswhere = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
        capture_output=True, text=True, check=True).stdout.strip()
    setup = Path(vs) / 'VC/Auxiliary/Build/vcvars64.bat'
    command = [str(snapshot / 'compiler/clang.exe'), '-std=gnu11', '-O1', '-ffp-contract=off',
        '-ffunction-sections', '-fdata-sections', '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES',
        '-D_CRT_SECURE_NO_WARNINGS', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
        '-I' + str(snapshot / 'sdk/src'), '-I' + str(snapshot / 'sdk/extern/dolphin/include'),
        str(output / 'compare.c'), str(snapshot / 'sdk/src/melee/ft/native_callback_lifetime.c'),
        str(snapshot / 'sdk/src/melee/ft/native_fighter_target.c'),
        str(HERE.parent / 'model_effect_absent_fixture.c'), '-fuse-ld=lld', '-Wl,/OPT:REF',
        '-o', str(output / 'compare.exe')]
    (output / 'compile.cmd').write_text('@echo off\ncall "' + str(setup) +
        '" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n' + subprocess.list2cmdline(command) + '\n')
    run(['cmd.exe', '/d', '/c', str(output / 'compile.cmd')], output / 'compile.log')
    syntax = command[:command.index(str(output / 'compare.c'))]
    syntax.extend(['-c', str(output / 'composition.generated.c'), '-o', str(output / 'composition.obj')])
    (output / 'composition-compile.cmd').write_text('@echo off\ncall "' + str(setup) +
        '" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n' + subprocess.list2cmdline(syntax) + '\n')
    run(['cmd.exe', '/d', '/c', str(output / 'composition-compile.cmd')], output / 'composition-compile.log')
    text = run([str(output / 'compare.exe')], output / 'result.log')
    match = re.search(r'Contracts source: (\d+) checks, (\d+) pairs; absorb(\d+) turn(\d+) pickup(\d+) food40', text)
    if not match or tuple(map(int, match.groups()[1:])) != (702, 250, 448, 4):
        raise RuntimeError('Unexpected coverage: ' + text)
    receipt = {'status': 'unregistered generic contracts and Oil Panic source comparison',
        'snapshot': str(snapshot), 'snapshot_sha256': sha(snapshot / 'freeze.json'),
        'original_inputs': sources, 'generated_sha256': sha(generated),
        'composition_generated_sha256': sha(output / 'composition.generated.c'),
        'composition_inputs': json.loads((output / 'composition/composition_inputs.json').read_text()),
        'composition_status': 'generated C compiles; new prop constructors and paper setup not runtime-compared',
        'fixture_sources': {p.name: sha(p) for p in HERE.iterdir() if p.is_file()},
        'results': dict(zip(['checks', 'pairs', 'absorb', 'turn', 'pickup'], map(int, match.groups()))),
        'food_lifetime_cases': 40,
        'typed_fighter_article_query_cases': 16,
        'stale_owner_pickup_safety_cases': 2,
        'boundaries': ['food allocation controlled; real common constructor tested separately',
            'common motion, facing, color, visibility and item animation controlled',
            'actual creation-ID target lookup and callback-source registry',
            'retired raw owner inputs have no stock source comparison',
            'paper rendering, real GameWatch base/import and whole-fighter activation excluded']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(text.strip())
    print('Receipt:', output / 'receipt.json')


if __name__ == '__main__':
    main()
