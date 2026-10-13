"""Frozen complete-special composition and original Transform source comparison."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from compose import compose

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
                      re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match:
        raise ValueError('Missing original ' + name)
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'snapshot', 'output'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--iso', type=Path)
    args = parser.parse_args()
    root, snapshot, output = args.root.resolve(), args.snapshot.resolve(), args.output.resolve()
    if output.drive.upper() != 'A:':
        parser.error('Output must be on A:')
    output.mkdir(parents=True, exist_ok=False)
    frozen = json.loads((snapshot / 'freeze.json').read_text())
    def verify():
        for path, expected in frozen.items():
            if sha(snapshot / path) != expected:
                raise RuntimeError('Changed snapshot input ' + path)
    verify()
    inputs = {str(path): sha(path) for path in HERE.rglob('*') if path.is_file() and '__pycache__' not in path.parts}
    for directory in ('sheik_clone_activation', 'sheik_chain_composition', 'sheik_script_composition',
                      'zelda_script_composition', 'zelda_travel58_composition'):
        inputs.update({str(path): sha(path) for path in (HERE.parent / directory).glob('*.fms')})
        inputs.update({str(path): sha(path) for path in (HERE.parent / directory).glob('compose.py')})
    sdk = snapshot / 'sdk'
    iso = args.iso or root.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso'
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    inputs[str(root / 'tools/extract_native_animation.py')] = sha(root / 'tools/extract_native_animation.py')
    commands, results = [], {}
    for member, short, data_root, states, slots in (
        ('sheik', 'Sk', 'ftDataSeak', 24, {'It_Kind_Seak_NeedleThrow': 0, 'It_Kind_Seak_NeedleHeld': 1,
                                       'It_Kind_Seak_Vanish': 2, 'It_Kind_Seak_Chain': 3}),
        ('zelda', 'Zd', 'ftDataZelda', 18, {'It_Kind_Zelda_DinFire': 0, 'It_Kind_Zelda_DinFire_Explode': 1})):
        out = output / member
        out.mkdir()
        dat, top = extract_clip(iso, 'Pl' + short + '.dat', data_root)
        word = lambda offset: struct.unpack_from('>I', dat, 32 + offset)[0]
        attributes, articles = word(top + 4), word(top + 72)
        source, owned, parameters, article = compose(member), [], [], None
        (out / 'candidate.fms').write_text(source, newline='\n')
        for line in source.splitlines():
            if line.startswith('article '):
                article = line.split()[1]
            literal = re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)', line)
            imported = re.fullmatch(r'(\s*)attribute (float|int|bool) (\w+) (\d+)', line)
            if not literal and not imported:
                owned.append(line.replace('item.attr.', 'item.params.'))
                continue
            if literal:
                indent, kind, name, value = literal.groups()
                offset = None
                bits = struct.unpack('>I', struct.pack('>f', float(value)))[0] if kind == 'float' else (
                    int(value.lower() == 'true') if kind == 'bool' else int(value, 0) & 0xffffffff)
            else:
                indent, kind, name, offset = imported.groups()
                offset = int(offset)
                base = word(word(articles + slots[article] * 4) + 4) if indent else attributes
                bits = dat[32 + base + offset] if kind == 'bool' else word(base + offset)
            value = ('true' if bits else 'false') if kind == 'bool' else repr(struct.unpack(
                '>f' if kind == 'float' else '>i', struct.pack('>I', bits))[0])
            parameters.append({'name': article + '.' + name if indent else name,
                               'type': kind, 'bits': bits, 'value': value,
                               'offset': offset, 'article': article if indent else None})
            owned.append('')
        (out / 'owned.fms').write_text('\n'.join(owned).rstrip() + '\n', newline='\n')
        (out / 'fighter_parameters.fmp').write_text(''.join(
            f"{p['type']} {p['name']} = {p['value']}\n" for p in parameters), newline='\n')
        def run(command, stage):
            command = list(map(str, command))
            completed = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=180)
            (out / (stage + '.log')).write_text(completed.stdout + completed.stderr, newline='\n')
            commands.append({'member': member, 'stage': stage, 'command': command, 'exit': completed.returncode})
            if completed.returncode:
                raise RuntimeError(stage + ':\n' + completed.stdout + completed.stderr)
            return completed.stdout
        run([snapshot / 'bin/native_content_tool.exe', 'script-c', out / 'owned.fms',
             'paired_' + member, out / 'generated.inc'], 'generate')
        generated = (out / 'generated.inc').read_text()
        count, schema = re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL', generated).groups()
        assert int(count) == len(parameters)
        (out / 'parameters.inc').write_text('static const uint32_t parameter_bits[] = {' + ','.join(
            f"UINT32_C({p['bits']})" for p in parameters) + '};\nstatic const uint64_t parameter_schema = ' + schema + 'ULL;\n', newline='\n')
        (out / 'attributes.inc').write_text('\n'.join(
            'memcpy((char*)&special_attributes+%d,&values[%d],%d);' %
            (p['offset'], index, 1 if p['type'] == 'bool' else 4)
            for index, p in enumerate(parameters) if p['offset'] is not None and p['article'] is None) + '\n', newline='\n')
        (out / 'tuning.inc').write_text('\n'.join(
            '{ float v=%sf; memcpy(&values[%d],&v,4); }' % (repr(value), index)
            for index, p in enumerate(parameters)
            for value in [{'transform_horizontal_divisor': 2.5, 'transform_vertical_divisor': 1.25,
                           'transform_gravity': 0.125, 'transform_terminal': 3.5,
                           'transform_finish_frame': 7.25}.get(p['name'])]
            if value is not None) + '\n', newline='\n')
        hosts = sorted(set(re.findall(r'host->(\w+) == 0', generated)))
        (out / 'hosts.inc').write_text('\n'.join(
            f'host.{name}=(__typeof__(host.{name}))unexpected;' for name in hosts) + '\n', newline='\n')
        folder = 'ftSeak' if member == 'sheik' else 'ftZelda'
        source_path = sdk / 'src/melee/ft/kinds' / folder / ('ftseakspeciallw.c' if member == 'sheik' else 'ftzeldaspeciallw.c')
        original = re.sub(r'^#include[^\n]*\n', '', source_path.read_text(), flags=re.M)
        if member == 'zelda':
            original = original[:original.index('s32 ftZd_SpecialLw_8013B540')]
        original += '\n' + body((sdk / 'src/melee/ft/ftmovement.c').read_text(), 'ftCommon_Fall')
        original += '\n#define SOLUTION 0\n' + body((sdk / 'src/melee/ft/ftmovement.c').read_text(), 'ftCommon_CalcSelfAccel_DeaccelAir')
        if member == 'zelda':
            common = (sdk/'src/melee/ft/kinds/ftCommon/ftCo_0A01.c').read_text()
            original += '\n' + body(common,'ftCo_CpuSetNeutralStick') + '\n' + body(common,'ftCo_800A949C')
        (out / 'original.inc').write_text(original + '\n', newline='\n')
        flags = [snapshot / 'compiler/clang.exe', '-std=gnu11', '-O2', '-ffp-contract=off', '-fms-runtime-lib=dll',
                 '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS', '-D_DLL', '-D_MT',
                 '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include'), '-I' + str(out)]
        def compile_(command, stage):
            batch = out / (stage + '.cmd')
            batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\n' +
                             subprocess.list2cmdline(list(map(str, command))) + '\n', newline='\n')
            return run(['cmd', '/c', batch], stage)
        compile_(flags + ['-shared', '-x', 'c', out / 'generated.inc', '-o', out / 'module.dll'], 'module')
        compile_(flags + ['-DMEMBER_SHEIK=' + str(int(member == 'sheik')), str(HERE / 'test.c'),
                         sdk / 'src/melee/ft/native_callback_lifetime.c', '-o', out / 'test.exe'], 'compile')
        counts = json.loads(run([out / 'test.exe'], 'compare'))
        results[member] = {'result': counts, 'parameters': parameters, 'dat_sha256': hashlib.sha256(dat).hexdigest(),
                           'declared_phase_capacity': states * 4,
                           'outputs': {p.name: sha(p) for p in out.iterdir() if p.is_file()}}
    verify()
    for path, expected in inputs.items():
        if sha(Path(path)) != expected:
            raise RuntimeError('Changed source ' + path)
    receipt = {'status': 'PASS unregistered composition compilation and Transform source scope',
               'snapshot_sha256': sha(snapshot / 'freeze.json'), 'inputs': inputs, 'commands': commands, 'members': results,
               'limits': ['Normal clone/import and paired registration remain pending; parameters use independently decoded retail DAT staging.',
                          'All accepted special components compile together. Executed new comparison covers Transform policies, not re-acceptance of every prior article/phase.',
                          'Common motion/stage/renderer/transfer are controlled boundaries; actual shared Fall and DeaccelAir and live registry execute.',
                          'Generated pending-action policies execute for both forms; Zelda recovery queues compare against actual shared source. Actual pending dispatcher and Ness/Yoshi travel migration have separate receipts.',
                          'Zelda Din CPU projectile-model translation and nearest-eligible-target query, full paired package registration/import, and whole lifecycle acceptance remain pending.',
                          'No behavior-independent declaration, activation, install or gameplay.']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', newline='\n')
    print(json.dumps({name: result['result'] for name, result in results.items()}))


if __name__ == '__main__':
    main()
