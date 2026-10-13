"""Unregistered GameWatch specials policy source comparison; no game."""
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
    for path in ['src/melee/ft/kinds/ftGameWatch/ftgamewatchspecials.c',
                 'src/melee/ft/kinds/ftGameWatch/ftgamewatchspecialhi.c',
                 'src/melee/ft/kinds/ftGameWatch/ftgamewatchspeciallw.c']:
        source = snapshot / 'sdk' / path
        originals[path] = sha(source)
        bodies += source.read_text() + '\n'
    functions = {
      'itgamewatchjudge.c':['it_802C78B8','itGameWatchJudge_Logic77_Destroyed','it_2725_Logic77_PickedUp','itGamewatchjudge_UnkMotion0_Anim','itGameWatchJudge_Logic77_EvtUnk','it_802C7AD0','it_802C7AF0'],
      'itgamewatchpanic.c':['itGameWatchPanic_Logic78_Destroyed','itGamewatchpanic_UnkMotion1_Anim','itGameWatchPanic_Logic78_EvtUnk','it_802C7EE0','it_802C7F00'],
      'itgamewatchrescue.c':['itGamewatchrescue_UnkMotion1_Anim','itGamewatchrescue_UnkMotion1_Phys','itGamewatchrescue_UnkMotion1_Coll','itGameWatchRescue_Logic81_EvtUnk','it_802C81C8','it_802C81E8']}
    for name, names in functions.items():
        path = 'src/melee/it/kinds/' + name
        source = snapshot / 'sdk' / path
        originals[path] = sha(source)
        bodies += ''.join(extract(source.read_text(), name) for name in names)
    pause = snapshot / 'sdk/src/melee/it/it_26B1.c'
    originals['src/melee/it/it_26B1.c'] = sha(pause)
    bodies += extract(pause.read_text(), 'it_8026B724') + extract(pause.read_text(), 'it_8026B73C')
    coll = snapshot / 'sdk/src/melee/ft/ftcoll.c'
    originals['src/melee/ft/ftcoll.c'] = sha(coll)
    bodies += extract(coll.read_text(), 'ftColl_8007AF10')
    (output / 'original.h').write_text(bodies, encoding='utf-8')
    if originals != expected_originals:
        raise RuntimeError('Original stock reference inputs changed; review source before updating receipt.')
    for name in ['candidate.fms', 'fighter_parameters.fmp', 'compare.c', 'article_compare.h']:
        shutil.copyfile(HERE / name, output / name)
    assembled = (HERE / 'fighter.fms').read_text().rstrip() + '\n' + (HERE / 'articles.fms').read_text()
    if assembled != (HERE / 'candidate.fms').read_text():
        raise RuntimeError('Candidate differs from its readable source components.')
    generated = output / 'behavior.generated.h'
    run([str(snapshot / 'bin/native_content_tool.exe'), 'script-c',
         str(output / 'candidate.fms'), 'gamewatch_specials_probe', str(generated)], output / 'generate.log')
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
    match = re.search(r'(\d+) checks, (\d+) failures, (\d+) pairs; phases(\d+) rolls(\d+) entries(\d+) articles(\d+)', text)
    if not match or int(match[2]) or tuple(map(int,match.groups()[2:])) != (45880,33280,10368,384,1848):
        raise RuntimeError('Unexpected comparison results: ' + text)
    receipt = {'status': 'unregistered partial specials source comparison',
               'snapshot': str(snapshot), 'snapshot_sha256': sha(snapshot / 'freeze.json'),
               'original_inputs': originals, 'generated_sha256': sha(generated),
               'fixture_sources': {p.name: sha(p) for p in HERE.iterdir() if p.is_file()},
               'model_effect_absent_fixture_sha256': sha(HERE.parent / 'model_effect_absent_fixture.c'),
               'results': dict(zip(['checks','failures','pairs','phases','rolls','entries','articles'],map(int,match.groups()))),
               'boundaries': ['shared engine physics/animation/collision controlled',
                              'three-prop allocation and paper renderer excluded'],
               'pending': ['absorber.hits RO Int', 'ordinary food constructor options/capacity',
                           'common horizontal deadzone query', 'current-owner airborne query',
                           'prop construction/setup', 'real base/import/rendering/gameplay']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(text.strip())
    print('Receipt:', output / 'receipt.json')


if __name__ == '__main__':
    main()
