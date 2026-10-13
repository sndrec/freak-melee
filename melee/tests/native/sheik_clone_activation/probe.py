"""Compile the unregistered owned base and supported partial composition."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import sys
from pathlib import Path
from compose import compose

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'snapshot', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--iso', type=Path)
    args = parser.parse_args()
    root, snapshot, output = args.root.resolve(), args.snapshot.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    iso = args.iso or root.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso'
    sdk = snapshot / 'sdk'
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    data, data_root = extract_clip(iso, 'PlSk.dat', 'ftDataSeak')
    word = lambda offset: struct.unpack_from('>I', data, 32 + offset)[0]
    special, items = word(data_root + 4), word(data_root + 72)
    slots = {'It_Kind_Seak_NeedleThrow': 0, 'It_Kind_Seak_NeedleHeld': 1,
             'It_Kind_Seak_Vanish': 2, 'It_Kind_Seak_Chain': 3}
    source = compose()
    (output / 'candidate.fms').write_text(source, encoding='utf-8', newline='\n')
    parameters, owned, article = [], [], None
    for line in source.splitlines():
        if line.startswith('article '):
            article = line.split()[1]
        literal = re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)', line)
        attribute = re.fullmatch(r'(\s*)attribute (float|int) (\w+) (\d+)', line)
        if literal:
            indent, kind, name, value = literal.groups()
            bits = struct.unpack('>I', struct.pack('>f', float(value)))[0] if kind == 'float' else int(value) & 0xffffffff
            offset = None
        elif attribute:
            indent, kind, name, offset = attribute.groups()
            offset = int(offset)
            attributes = word(word(items + slots[article] * 4) + 4) if indent else special
            bits = word(attributes + offset)
            value = repr(struct.unpack('>f' if kind == 'float' else '>i', struct.pack('>I', bits))[0])
        else:
            owned.append(line.replace('item.attr.', 'item.params.'))
            continue
        parameters.append({'name': article + '.' + name if indent else name, 'type': kind,
                           'value': value, 'bits': bits, 'source_offset': offset})
        owned.append('')
    (output / 'owned.fms').write_text('\n'.join(owned).rstrip() + '\n', encoding='utf-8')
    (output / 'fighter_parameters.fmp').write_text(''.join(
        f"{p['type']} {p['name']} = {p['value']}\n" for p in parameters), encoding='utf-8')
    commands = []

    def run(command, name):
        command = list(map(str, command))
        commands.append(command)
        result = subprocess.run(command, cwd=root, text=True, capture_output=True)
        (output / name).write_text(result.stdout + result.stderr, encoding='utf-8')
        if result.returncode:
            raise RuntimeError(name + '\n' + (result.stdout + result.stderr)[-5000:])

    tool, clang = snapshot / 'bin/native_content_tool.exe', snapshot / 'compiler/clang.exe'
    run([tool, 'script-c', output / 'owned.fms', 'sheik_clone', output / 'generated.c'], 'generate.log')
    # Quoted stock includes resolve beside the owned C input first. Prove their
    # bytes match the immutable SDK before compilation; no mixed live ABI.
    originals = list((root / 'src/melee/ft/kinds/ftSeak').glob('*.h'))
    for header in originals:
        if header.name == 'ftsheik_clone.h':
            continue
        assert sha(header) == sha(sdk / header.relative_to(root)), header
    flags = [clang, '-std=gnu11', '-O2', '-DMELEE_NATIVE=1', '-D_DLL', '-D_MT',
        '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS', '-fms-runtime-lib=dll',
        '-I', sdk / 'src', '-I', sdk / 'extern/dolphin/include']

    def compile_(arguments, name):
        batch = output / (name + '.cmd')
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\n'
                         + subprocess.list2cmdline(list(map(str, arguments))) + '\n', encoding='utf-8')
        run(['cmd', '/c', batch], name + '.log')

    compile_(flags + ['-fsyntax-only', root / 'src/melee/ft/kinds/ftSeak/ftsheik_clone.c'], 'owned_base')
    compile_(flags + ['-shared', output / 'generated.c', '-o', output / 'candidate.dll'], 'generated_module')
    inputs = [HERE / 'compose.py', HERE / 'lifecycle.fms', HERE / 'probe.py',
        root / 'src/melee/ft/kinds/ftSeak/ftsheik_clone.c', root / 'src/melee/ft/kinds/ftSeak/ftsheik_clone.h',
        HERE.parent / 'sheik_chain_composition/fighter.fms', HERE.parent / 'sheik_chain_composition/article.fms',
        HERE.parent / 'sheik_chain_composition/compose.py', HERE.parent / 'sheik_script_composition/needles.fms',
        HERE.parent / 'sheik_script_composition/vanish.fms']
    receipt = {'status': 'PASS owned base syntax and partial generated module compilation; no activation',
        'snapshot': str(snapshot), 'freeze_sha256': sha(snapshot / 'freeze.json'), 'parameters': parameters,
        'inputs': {str(path.relative_to(root)): sha(path) for path in inputs},
        'tools': {'content_tool': sha(tool), 'clang': sha(clang)}, 'commands': commands,
        'outputs': {name: sha(output / name) for name in ('candidate.fms', 'owned.fms', 'fighter_parameters.fmp', 'generated.c', 'candidate.dll')},
        'limits': ['Fighter phases341..360 and all four article components compile; Transform361..364 deliberately absent.',
                   'No normal project clone/import, registry, module-entry independence audit, asset runtime registration or gameplay accepted.',
                   'Lifecycle handlers compile; actual original/generated lifecycle comparison is still required.']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(output / 'receipt.json')


if __name__ == '__main__':
    main()
