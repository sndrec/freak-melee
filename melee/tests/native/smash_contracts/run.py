"""Compile isolated semantic charge contracts against a matching immutable snapshot."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import subprocess

HERE = Path(__file__).resolve().parent
SOURCE = 'src/melee/ft/ft_0DF0.c'
FUNCTIONS = ['ftCo_800DEE84', 'ftCo_800DEEA8', 'ftCo_800DEEB8',
             'ftCo_800DEEE8', 'ftCo_800DEF38', 'ftCo_800DF0D0']

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def extract(text, name):
    match = re.search(r'(?m)^[A-Za-z_][^\n;{}]*\b'+re.escape(name)+r'\([^;]*?\)\s*\{', text)
    if not match:
        raise RuntimeError('Original function not found: '+name)
    start = text.index('{', match.start())
    end, depth = start+1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]+'\n\n'

def run(command, log):
    result = subprocess.run(command, capture_output=True, text=True)
    log.write_text(result.stdout+result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'Failed({result.returncode}): {log}\n'+result.stdout+result.stderr)
    return result.stdout

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if any(output == p or p in output.parents for p in [snapshot, HERE]):
        raise RuntimeError('Output cannot modify snapshot or tracked source.')
    frozen = {k.replace('\\', '/'): v for k,v in json.loads((snapshot/'freeze.json').read_text()).items()}
    for key,value in frozen.items():
        if key.startswith(('sdk/', 'compiler/')) or key == 'bin/native_content_tool.exe':
            if sha(snapshot/key) != value:
                raise RuntimeError('Snapshot hash mismatch: '+key)
    expected = json.loads((HERE/'original_inputs.json').read_text())
    original = snapshot/'sdk'/SOURCE
    if sha(original) != expected[SOURCE]:
        raise RuntimeError('Original common charge source changed; review before repinning.')
    output.mkdir(parents=True, exist_ok=True)
    source_text = original.read_text()
    extracted = ''.join(extract(source_text, name) for name in FUNCTIONS)
    (output/'original.h').write_text(extracted, encoding='utf-8')
    vswhere = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    install = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
        capture_output=True, text=True, check=True).stdout.strip()
    setup = Path(install)/'VC/Auxiliary/Build/vcvars64.bat'
    results = {}
    for mode in ['action', 'callback']:
        directory = output/mode
        directory.mkdir(exist_ok=True)
        source = (HERE/'contract.fms').read_text()
        if mode == 'callback':
            source = 'base ftDataNess 341 1\n'+source.replace('action SmashContract:', 'move SmashContract:').replace('    on load:', '    on state_341_physics:')
        (directory/'behavior.fms').write_text(source, encoding='utf-8')
        run([str(snapshot/'bin/native_content_tool.exe'), 'script-c', str(directory/'behavior.fms'),
             'smash_contract', str(directory/'behavior.generated.h')], directory/'generate.log')
        command = [str(snapshot/'compiler/clang.exe'), '-std=gnu11', '-O1', '-ffp-contract=off',
            '-ffunction-sections', '-fdata-sections', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
            '-D_USE_MATH_DEFINES', '-D_DLL', '-D_MT', '-DSMASH_CALLBACK_ROUTE='+str(int(mode == 'callback')),
            '-Xclang', '--dependent-lib=msvcrt', '-I'+str(snapshot/'sdk/src'),
            '-I'+str(snapshot/'sdk/extern/dolphin/include'), '-I'+str(directory), '-I'+str(output),
            str(HERE/'compare.c'), '-fuse-ld=lld', '-Wl,/OPT:REF', '-o', str(directory/'compare.exe')]
        batch = directory/'compile.cmd'
        batch.write_text('@echo off\ncall "'+str(setup)+'" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(command)+'\n', encoding='utf-8')
        run(['cmd.exe', '/d', '/c', str(batch)], directory/'compile.log')
        result = run([str(directory/'compare.exe')], directory/'result.log')
        match = re.search(r'(\d+) checks, (\d+) failures, (\d+) common source pairs, (\d+) bounded write cases', result)
        if not match or tuple(map(int,match.groups()[1:])) != (0,240,60):
            raise RuntimeError('Unexpected result scope: '+result)
        results[mode] = {'checks': int(match[1]), 'failures': 0, 'source_pairs': 240,
                         'bounded_write_cases': 60, 'generated_sha256': sha(directory/'behavior.generated.h')}
        print(mode+': '+result.strip())
    receipt = {'status': 'isolated common charge storage/source acceptance; no fighter smash parity claim',
        'snapshot': str(snapshot), 'snapshot_sha256': sha(snapshot/'freeze.json'),
        'originals': expected, 'original_fragment_sha256': sha(output/'original.h'),
        'fixture_sources': {p.name: sha(p) for p in HERE.iterdir() if p.is_file()}, 'results': results,
        'boundaries': ['ordered animation rate/color/rumble/sound delivery', 'controlled common tuning and shake table'],
        'not_covered': ['whole Ness seven-state smash translation', 'rendered gameplay', 'engine update dispatcher timing',
                        'smash hitboxes/victim histories/ECB swept queries']}
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print('Receipt:', output/'receipt.json')

if __name__ == '__main__':
    main()
