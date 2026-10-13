"""Fresh frozen-SDK Peach clone/import/DLL/selection/package gate; no game or install."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import sys
from pathlib import Path
from assemble import compose, COMPONENT
from fixture import extract
sys.path.insert(0, str(Path(__file__).resolve().parent.parent / 'peach_activation_composition'))
from audit import inspect

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE.parent))
from fighter_script_bundle_roundtrip import portrait, bundled_id


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def declaration(text, marker):
    start = text.index(marker)
    begin = text.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:text.index(';', end) + 1]


def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
                      re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match:
        raise ValueError('actual definition missing: ' + name)
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--guard-source', type=Path,
                        help='Explicit separate updated common source; only its guarded taunt entry is tested for safety')
    parser.add_argument('--iso', type=Path, default=ROOT.parent /
                        'Super Smash Bros. Melee (USA) (v1.02).iso')
    args = parser.parse_args()
    snap, out = args.snapshot.resolve(), args.output.resolve()
    if out.exists() or any(out == p or p in out.parents for p in [snap, HERE]):
        parser.error('use a fresh output outside the immutable inputs')
    manifest = json.loads((snap / 'freeze.json').read_text())

    def verify():
        for name, digest in manifest.items():
            if name.replace('\\', '/').startswith(('sdk/', 'compiler/', 'bin/')):
                if sha(snap / name) != digest:
                    raise RuntimeError('frozen input changed: ' + name)

    verify()
    out.mkdir(parents=True)
    sdk = snap / 'sdk'
    tool, clang = snap / 'bin/native_content_tool.exe', snap / 'compiler/clang.exe'
    commands = []

    def run(stage, command):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=240)
        (out / (stage + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8', newline='\n')
        commands.append({'stage': stage, 'command': command, 'exit': result.returncode})
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8', newline='\n')
        if result.returncode:
            raise RuntimeError(stage + ' failed:\n' + (result.stdout + result.stderr)[-7000:])
        return result.stdout

    source = compose()
    candidate = out / 'candidate.fms'
    candidate.write_text(source, encoding='utf-8', newline='\n')
    project = out / 'project'
    run('clone', [tool, 'clone-stock', args.iso.resolve(), 'Peach', project, 'Unregistered independent Peach'])
    run('import', [tool, 'import-parameters', project, candidate, project / 'behavior.fms'])
    key = re.search(r'^code=(\S+)$', (project / 'project.txt').read_text(), re.M)[1]
    generated, dll = project / 'behavior.generated.c', project / 'module.dll'
    run('generate', [tool, 'script-c', project / 'behavior.fms', key, generated])
    code = generated.read_text()
    assert 'generated.source_kind = Ft_Kind_None;' in code
    for token in ['fighter->u.pe', 'fighter->mv.pe', 'itemVar.peach', 'ftPe_']:
        assert token not in code, token
    flags = [clang, '-std=gnu11', '-O2', '-ffp-contract=off', '-fms-runtime-lib=dll',
             '-DMELEE_NATIVE=1', '-D_DLL', '-D_MT', '-D_USE_MATH_DEFINES',
             '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include')]
    vcvars = 'C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat'

    def compile(stage, command):
        batch = out / (stage + '.cmd')
        batch.write_text('@echo off\ncall "' + vcvars + '" >nul\n' +
                         subprocess.list2cmdline(list(map(str, command))) + '\n', encoding='utf-8', newline='\n')
        run(stage, ['cmd', '/c', batch])

    compile('dll', flags + ['-shared', generated, '-o', dll])
    stock = sdk / 'src/melee/ft/kinds/ftPeach/ftpeach.c'
    clone = sdk / 'src/melee/ft/kinds/ftPeach/ftpeach_clone.c'
    table = declaration(stock.read_text(), 'MotionState ftPe_Init_MotionStateTable')
    base = '\n'.join(declaration(clone.read_text(), marker) for marker in [
        'static const NativeFighterBone ftPeachClone_Bones',
        'static const NativeFighterArticleAsset ftPeachClone_Articles',
        'static const Fighter_DemoStrings ftPeachClone_DemoMotionNames',
        'const NativeFighterDefinition ftPeachClone_Definition'])
    names = sorted(set(re.findall(r'\bft(?:Pe|Co)_\w+\b', table + base)))
    names = [n for n in names if not any(t in n for t in ['_MS_', '_MF_', '_SM_', '_MotionStateTable'])]
    names += ['ftCamera_UpdateCameraBox']
    (out / 'base.h').write_text('\n'.join('#define ' + n + ' stock_slot' for n in names) + '\n' +
                              table + base + '\n' + '\n'.join('#undef ' + n for n in names), encoding='utf-8', newline='\n')
    definition = (sdk / 'src/melee/ft/native_fighter_runtime.h').read_text().split(
        'typedef struct NativeFighterDefinition {', 1)[1].split('} NativeFighterDefinition;', 1)[0]
    callbacks = sorted(set(re.findall(r'\(\*(\w+)\)', definition) +
                           re.findall(r'\b(?:HSD_GObjEvent|HSD_GObjPredicate|Fighter_ItemEvent)\s+(\w+)\s*;', definition)))
    (out / 'definition_callbacks.h').write_text(''.join('VERIFY_CALLBACK(' + n + ');\n' for n in callbacks), encoding='utf-8', newline='\n')
    required = sorted(set(re.findall(r'host->(\w+)\s*==\s*0', code)) - {'common_motion_states', 'debug_level'})
    (out / 'required_host.h').write_text('\n'.join('host.' + n + '=(__typeof__(host.' + n + '))unexpected;' for n in required), encoding='utf-8', newline='\n')
    params = []
    for line in (project / 'fighter_parameters.fmp').read_text().splitlines():
        typ, name, equal, value = line.split()
        assert equal == '='
        bits = struct.unpack('<I', struct.pack('<f', float(value)))[0] if typ == 'float' else int(value, 0) & 0xffffffff
        params.append((name, bits))
    schema = re.search(r'schema != (0x[\da-f]+)ULL', code)[1]
    (out / 'parameters.h').write_text('static const uint32_t parameter_bits[] = {' +
        ','.join(hex(bits) + 'u' for _, bits in params) + '};\n#define PARAMETER_SCHEMA ' + schema + 'ULL\n', encoding='utf-8', newline='\n')
    select = sdk / 'src/melee/it/native_item_behavior.c'
    extracted = body(select.read_text(), 'nativeItemBehavior_Select')
    (out / 'selection.h').write_text(extracted, encoding='utf-8', newline='\n')
    lifecycle = '\n'.join(body(stock.read_text(), name) for name in [
        'ftPe_Init_OnDeath', 'ftPe_Init_OnLoad', 'ftPe_Init_LoadSpecialAttrs'])
    (out / 'lifecycle.h').write_text(lifecycle, encoding='utf-8', newline='\n')
    exe = out / 'ownership.exe'
    compile('ownership_compile', flags + ['-I' + str(out), HERE / 'ownership.c', '-o', exe])
    result = json.loads(run('ownership', [exe, dll]))
    extraction = extract(sdk, out, args.guard_source)
    taunt_exe = out / 'taunt.exe'
    compile('taunt_compile', flags + ['-I' + str(out), HERE / 'taunt_test.c', sdk / 'src/melee/ft/native_callback_lifetime.c', HERE.parent / 'model_effect_absent_fixture.c', '-o', taunt_exe])
    taunt = json.loads(run('taunt', [taunt_exe, dll]))
    audit = inspect(snap, project, candidate, project / 'fighter_parameters.fmp')
    (out / 'audit.json').write_text(json.dumps(audit, indent=2) + '\n', encoding='utf-8', newline='\n')
    image = out / 'portrait.png'
    portrait(image)
    store = out / 'isolated-store'
    package = bundled_id(run('bundle', [tool, '--root', store, 'bundle-project', project, image, args.iso.resolve()]))
    run('verify_package', [tool, '--root', store, 'verify-package', package, args.iso.resolve()])
    verify()
    inputs = [HERE / 'prepare.py', HERE / 'assemble.py', HERE.parent / 'peach_activation_composition/audit.py', HERE / 'ownership.c', COMPONENT,
              HERE.parent / 'fighter_script_bundle_roundtrip.py', HERE / 'fixture.py', HERE / 'taunt_test.c', HERE / 'taunt.fms', out / 'taunt_bodies.h', sdk / 'src/melee/ft/native_callback_lifetime.c', HERE.parent / 'model_effect_absent_fixture.c', stock, clone, select,
              generated, dll, candidate, project / 'behavior.fms', project / 'fighter_parameters.fmp',
              project / 'project.txt', *[project / n for n in ['fighter.dat', 'animation.dat', 'costume.dat', 'cpu_profile.fcp']],
              *[out / n for n in ['base.h', 'definition_callbacks.h', 'required_host.h', 'parameters.h', 'selection.h', 'lifecycle.h']]]
    (out / 'receipt.json').write_text(json.dumps({'status': 'PASS unregistered independent module/asset-selection/package gate; no gameplay or rendering claim',
        'snapshot': str(snap), 'snapshot_sha256': sha(snap / 'freeze.json'), 'candidate_parameter_count': len(params),
        'results': result, 'taunt': taunt, 'taunt_extraction': extraction, 'raw_dat_words_verified': len(audit['raw_attribute_words_verified']),
        'audit_sha256': sha(out / 'audit.json'), 'package_id': package, 'commands': commands,
        'source_sha256': {str(p): sha(p) for p in inputs}}, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(out / 'receipt.json')


if __name__ == '__main__':
    main()
