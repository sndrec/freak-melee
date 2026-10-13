"""Frozen SDK59 source comparison, never starts a game or uses script-build."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
from compose import ACCEPTED, HERE, compose, load


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if output.is_relative_to(snapshot) or output.is_relative_to(HERE):
        raise RuntimeError('Outputs must be separate from immutable inputs')
    output.mkdir(parents=True, exist_ok=True)
    frozen = json.loads((snapshot / 'freeze.json').read_text())
    for relative, digest in frozen.items():
        if sha(snapshot / relative) != digest: raise RuntimeError('Snapshot hash: ' + relative)
    previous = load(ACCEPTED / 'run.py', 'accepted_normal_runner')
    inputs, bodies = {}, []
    for group, names in [('ft', previous.FIGHTERS), ('it', previous.ARTICLES)]:
        for name in names:
            relative = ('src/melee/ft/kinds/ftGameWatch/' if group == 'ft' else 'src/melee/it/kinds/') + name
            path = snapshot / 'sdk' / relative
            inputs[relative] = sha(path)
            source = path.read_text()
            # All fighter normal entries/helpers/phases, and all non-constructor
            # article callbacks, retain unchanged source bytes.
            functions = re.findall(r'(?m)^(?:static inline |static )?(?:void|bool) ([A-Za-z0-9_]+)\([^;]*?\)\s*\{', source)
            bodies.extend(previous.extract(source, function) for function in functions)
    for relative, names in previous.EXTRACTED.items():
        path = snapshot / 'sdk' / relative
        inputs[relative] = sha(path)
        bodies.extend(previous.extract(path.read_text(), name) for name in names)
    shared = {
        'ftCo_Attack1.c': ['onPkPc21EC', 'getMotionFlags', 'ftCo_Attack11_EnterMotion'],
        'ftCo_Attack100.c': ['ftCo_800D6B00', 'ftCo_800D6C60', 'ftCo_Attack100End_Anim'],
        'ftCo_AttackAir.c': ['ftCo_AttackAir_Anim'],
        'ftCo_Landing.c': ['ftCo_Landing_Anim', 'ftCo_Landing_Phys', 'ftCo_Landing_Coll'],
        'ftCo_LandingAir.c': ['ftCo_LandingAir_EnterWithMsidLag'],
    }
    for name, names in shared.items():
        relative = 'src/melee/ft/kinds/ftCommon/' + name
        path = snapshot / 'sdk' / relative
        inputs[relative] = sha(path)
        bodies.extend(previous.extract(path.read_text(), name) for name in names)
    if inputs != json.loads((HERE / 'original_inputs.json').read_text()):
        raise RuntimeError('Original source manifest changed')
    headers = [p.stem + '.h' for p in map(Path, inputs) if '/ftGameWatch/' in str(p).replace('\\', '/')]
    headers += [name.replace('.c', '.h') for name in previous.ARTICLES]
    headers += ['itgamewatchjudge.h', 'itgamewatchpanic.h', 'itgamewatchrescue.h']
    headers += [name.replace('.c', '.h') for name in shared]
    declarations = ''.join(body[:body.index('{')].strip() + ';\n' for body in bodies)
    prefix = ''.join('#include "' + h + '"\n' for h in headers) + declarations
    (output / 'original_declarations.h').write_text(prefix)
    (output / 'original.h').write_text(prefix + '\n'.join(bodies))
    # Reuse the accepted fixture's controlled engine boundaries and comparison
    # helpers as source, never its test loops. Pin the full dependency hash.
    support = (ACCEPTED / 'compare.c').read_text()
    for name in ['main', 'articles', 'cleanup', 'accessories', 'resets', 'lb_8000B1CC', 'lb_800119DC']:
        support = support.replace(previous.extract(support, name).rstrip(), '')
    support = support.replace('#include "original.h"', '#include "original_declarations.h"')
    (output / 'support.h').write_text(support.rstrip() + '\n')
    candidate = compose(output)
    generated = output / 'behavior.generated.h'
    previous.run([str(snapshot / 'bin/native_content_tool.exe'), 'script-c', str(candidate),
                  'gamewatch_normal_entries_probe', str(generated)], output / 'generate.log')
    text = generated.read_text()
    schema = re.search(r'schema != (0x[0-9a-f]+)ULL', text)[1]
    bits = []
    for line in (output / 'fighter_parameters.fmp').read_text().splitlines():
        kind, name, equals, value = line.split()
        assert equals == '='
        bits.append(str(struct.unpack('<I', struct.pack('<f', float(value)))[0] if kind == 'float' else int(value) & 0xFFFFFFFF))
    (output / 'parameters.h').write_text('#define FIXTURE_PARAMETER_COUNT ' + str(len(bits)) +
        '\n#define FIXTURE_PARAMETER_SCHEMA ' + schema + 'ULL\nstatic uint32_t fixture_parameter_bits[]={' + ','.join(bits) + '};\n')
    hosts = sorted(set(re.findall(r'host->([a-zA-Z0-9_]+) == 0', text)))
    (output / 'unused_host_functions.h').write_text(''.join(
        'static void unused_' + h + '(void){fprintf(stderr,"Unexecuted Host ' + h + '\\n");abort();}\n' for h in hosts))
    (output / 'unused_hosts.h').write_text(''.join('host.' + h + '=(__typeof__(host.' + h + '))unused_' + h + ';\n' for h in hosts))
    for name in ['compare.c', 'boundaries.h', 'matrix.h']:
        shutil.copyfile(HERE / name, output / name)
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
        '-I' + str(snapshot / 'sdk/src/melee/ft/kinds/ftCommon'),
        '-I' + str(snapshot / 'sdk/src/melee/it/kinds'), str(output / 'compare.c'),
        str(snapshot / 'sdk/src/melee/ft/native_callback_lifetime.c'),
        str(HERE.parent / 'model_effect_absent_fixture.c'), '-fuse-ld=lld', '-Wl,/OPT:REF', '-o', str(output / 'compare.exe')]
    (output / 'compile.cmd').write_text('@echo off\ncall "' + str(setup) + '" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n' + subprocess.list2cmdline(command) + '\n')
    previous.run(['cmd.exe', '/d', '/c', str(output / 'compile.cmd')], output / 'compile.log')
    result_text = previous.run([str(output / 'compare.exe')], output / 'result.log')
    match = re.search(r'Normal entries: (\d+) checks, (\d+) failures, (\d+) pairs; phases(\d+) entries(\d+) setup(\d+) wind(\d+)', result_text)
    if not match or int(match[2]): raise RuntimeError('Source matrix failed: ' + result_text)
    result = dict(zip(['checks', 'failures', 'pairs', 'phases', 'entries', 'setup', 'wind'], map(int, match.groups())))
    assert result['phases'] == 20666 and result['entries'] == 4618 and result['setup'] == 24576 and result['wind'] == 100, result
    receipt = {'status': 'unregistered SDK59 normal entry/phase and Manhole source comparison',
        'snapshot': str(snapshot), 'snapshot_sha256': sha(snapshot / 'freeze.json'),
        'original_inputs': inputs, 'composition_inputs': json.loads((output / 'composition_inputs.json').read_text()),
        'fixture_sources': {p.name: sha(p) for p in HERE.iterdir() if p.is_file()},
        'absent_effect_registry_fixture_sha256': sha(HERE.parent / 'model_effect_absent_fixture.c'),
        'generated_sha256': sha(generated), 'results': result,
        'boundaries': ['controlled motion, animation, input, collision and constructor tuple',
                       'actual extracted common jab/rapid/landing startup and animation bodies',
                       'actual creation-ID source lifetime registry', 'one controlled Vec3 world read and common wind call'],
        'excluded': ['paper rendering/asset/base registration', 'actual allocation/attachment',
                     'full shared collision/input solver', 'foreign/reflected prop ownership', 'games/default activation']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(result_text.strip())
    print('Receipt:', output / 'receipt.json')


if __name__ == '__main__': main()
