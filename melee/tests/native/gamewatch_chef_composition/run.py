"""Unregistered Chef/food source comparison; controlled presentation, no game."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, log):
    result = subprocess.run(command, capture_output=True, text=True)
    log.write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(f'Failed ({result.returncode}): {log}\n{result.stdout}{result.stderr}')
    return result.stdout


def extract(source, name):
    match = re.search(r'(?m)^[A-Za-z_][^\n;{}]*\b' + re.escape(name) +
                      r'\([^;]*?\)\s*\{', source)
    if not match:
        raise RuntimeError('Missing original: ' + name)
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if any(output == p or p in output.parents for p in [snapshot, HERE]):
        raise RuntimeError('Output must not modify snapshot or source namespace.')
    frozen = json.loads((snapshot / 'freeze.json').read_text())
    for key, expected in frozen.items():
        path = snapshot / key.replace('\\', '/')
        if sha(path) != expected:
            raise RuntimeError('Snapshot input changed: ' + key)
    output.mkdir(parents=True, exist_ok=True)
    originals = {}
    expected_originals = json.loads((HERE / 'original_inputs.json').read_text())
    bodies = ''
    for path in ['src/melee/ft/kinds/ftGameWatch/ftgamewatchspecialn.c',
                 'src/melee/it/kinds/itgamewatchchef.c']:
        source = snapshot / 'sdk' / path
        originals[path] = sha(source)
        bodies += source.read_text() + '\n'
    gravity = snapshot / 'sdk/src/melee/it/it_2725.c'
    originals['src/melee/it/it_2725.c'] = sha(gravity)
    bodies += extract(gravity.read_text(), 'it_80272860')
    (output / 'original.h').write_text(bodies, encoding='utf-8')
    rng = snapshot / 'sdk/src/melee/ft/native_fighter_module.c'
    originals['src/melee/ft/native_fighter_module.c'] = sha(rng)
    (output / 'random_original.h').write_text(extract(rng.read_text(), 'random_int_excluding'))
    if originals != expected_originals:
        raise RuntimeError('Original stock reference inputs changed; review source before updating receipt.')
    for name in ['candidate.fms', 'fighter_parameters.fmp', 'compare.c']:
        shutil.copyfile(HERE / name, output / name)
    generated = output / 'behavior.generated.h'
    run([str(snapshot / 'bin/native_content_tool.exe'), 'script-c',
         str(output / 'candidate.fms'), 'gamewatch_chef_probe', str(generated)], output / 'generate.log')
    schema = re.search(r'schema != (0x[0-9a-f]+)ULL', generated.read_text())[1]
    bits, names = [], []
    for line in (HERE / 'fighter_parameters.fmp').read_text().splitlines():
        kind, name, equals, value = line.split()
        assert equals == '='
        names.append('"' + name + '"')
        bits.append(str(struct.unpack('<I', struct.pack('<f', float(value)))[0]
                        if kind == 'float' else int(value) & 0xFFFFFFFF))
    (output / 'parameters.h').write_text(
        '#define FIXTURE_PARAMETER_COUNT ' + str(len(bits)) + '\n' +
        '#define FIXTURE_PARAMETER_SCHEMA ' + schema + 'ULL\n' +
        'static uint32_t fixture_parameter_bits[] = {' + ','.join(bits) + '};\n' +
        'static const char* fixture_parameter_names[] = {' + ','.join(names) + '};\n')
    vswhere = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    vs = subprocess.run([str(vswhere), '-latest', '-products', '*', '-requires',
                         'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
                        capture_output=True, text=True, check=True).stdout.strip()
    setup = Path(vs) / 'VC/Auxiliary/Build/vcvars64.bat'
    command = [str(snapshot / 'compiler/clang.exe'), '-std=gnu11', '-O1', '-ffp-contract=off',
               '-ffunction-sections', '-fdata-sections', '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES',
               '-D_CRT_SECURE_NO_WARNINGS', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
               '-I' + str(snapshot / 'sdk/src'), '-I' + str(snapshot / 'sdk/extern/dolphin/include'),
               '-I' + str(snapshot / 'sdk/src/melee/ft/kinds/ftGameWatch'),
               '-I' + str(snapshot / 'sdk/src/melee/it/kinds'),
               str(output / 'compare.c'), str(snapshot / 'sdk/src/melee/ft/native_callback_lifetime.c'),
               str(HERE.parent / 'model_effect_absent_fixture.c'),
               '-fuse-ld=lld', '-Wl,/OPT:REF', '-o', str(output / 'compare.exe')]
    (output / 'compile.cmd').write_text('@echo off\ncall "' + str(setup) +
        '" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n' + subprocess.list2cmdline(command) + '\n')
    run(['cmd.exe', '/d', '/c', str(output / 'compile.cmd')], output / 'compile.log')
    text = run([str(output / 'compare.exe')], output / 'result.log')
    match = re.search(r'(\d+) checks, (\d+) failures, (\d+) pairs; fighter(\d+) food(\d+) edited(\d+)', text)
    if not match or int(match[2]) or tuple(map(int, match.groups()[2:])) != (38070, 7166, 30254, 650):
        raise RuntimeError('Unexpected comparison results: ' + text)
    receipt = {'status': 'unregistered Chef/food source comparison only',
               'snapshot': str(snapshot), 'snapshot_sha256': sha(snapshot / 'freeze.json'),
               'original_inputs': originals, 'generated_sha256': sha(generated),
               'model_effect_absent_fixture_sha256': sha(HERE.parent / 'model_effect_absent_fixture.c'),
               'fixture_sources': {p.name: sha(p) for p in HERE.iterdir() if p.is_file()},
               'results': dict(zip(['checks', 'failures', 'pairs', 'fighter', 'food', 'edited'], map(int, match.groups()))),
               'boundaries': ['HSD allocation and world matrix', 'animation evaluator and state transition',
                              'stage mask and shield interaction', 'paper colors/pass renderer excluded'],
               'pending': ['own real GameWatch base/import', 'layered rendering metadata and authored palette',
                           'all other fighter moves, props and complete lifecycle', 'drawn gameplay']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(text.strip())
    print('Receipt:', output / 'receipt.json')


if __name__ == '__main__':
    main()
