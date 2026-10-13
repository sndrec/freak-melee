"""Console ownership/lifecycle gate, with honest normal-clone rejection evidence."""
import argparse
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys

from audit import sha

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def body(text, name):
    match = re.search(r'(?m)^void '+re.escape(name)+r'\([^;{}]*\)\s*\{', text)
    if not match:
        raise RuntimeError('Missing original '+name)
    depth, end = 1, match.end()
    while depth:
        depth += (text[end] == '{')-(text[end] == '}')
        end += 1
    return text[match.start():end]


def stage(source, dat, top, slots, output):
    # Independent retail import boundary, not a substitute for normal project
    # import/package validation. The real clone command remains in the census.
    import struct
    word = lambda offset: struct.unpack_from('>I', dat, 32+offset)[0]
    attrs, articles = word(top+4), word(top+72)
    owned, values, parameter_text, article = [], [], [], None
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
            bits = struct.unpack('>I', struct.pack('>f', float(value)))[0] if kind == 'float' else (
                int(value.lower() == 'true') if kind == 'bool' else int(value, 0)&0xffffffff)
        else:
            indent, kind, name, offset = imported.groups()
            base = word(word(articles+slots[article]*4)+4) if indent else attrs
            bits = dat[32+base+int(offset)] if kind == 'bool' else word(base+int(offset))
        values.append(bits)
        value = ('true' if bits else 'false') if kind == 'bool' else repr(struct.unpack(
            '>f' if kind == 'float' else '>i', struct.pack('>I', bits))[0])
        parameter_text.append(f"{kind} {article+'.' if indent else ''}{name} = {value}\n")
        owned.append('')
    (output/'owned.fms').write_text('\n'.join(owned).rstrip()+'\n', newline='\n')
    (output/'fighter_parameters.fmp').write_text(''.join(parameter_text), newline='\n')
    return values


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('snapshot', 'iso', 'output'):
        p.add_argument('--'+name, required=True, type=Path)
    args = p.parse_args()
    snapshot, out = args.snapshot.resolve(), args.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        p.error('Use a fresh output on A:')
    out.mkdir(parents=True)
    inputs = {str(p):sha(p) for p in HERE.rglob('*') if p.is_file() and '__pycache__' not in p.parts}
    for directory in ('sheik_clone_activation','sheik_chain_composition','sheik_script_composition',
            'zelda_script_composition','zelda_travel58_composition','paired_form_composition','zelda_din_cpu'):
        inputs.update({str(p):sha(p) for p in (HERE.parent/directory).iterdir()
            if p.is_file() and p.suffix in ('.py','.fms')})
    inputs[str(ROOT/'tools/extract_native_animation.py')] = sha(ROOT/'tools/extract_native_animation.py')
    commands = []
    def run(cmd, directory, stage):
        cmd = list(map(str, cmd))
        r = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True, timeout=240)
        (directory/(stage+'.log')).write_text(r.stdout+r.stderr, newline='\n')
        commands.append({'command':cmd, 'stage':stage, 'exit':r.returncode})
        if r.returncode:
            raise RuntimeError(stage+'\n'+(r.stdout+r.stderr)[-6000:])
        return r.stdout
    run([sys.executable, HERE/'audit.py', '--snapshot', snapshot, '--iso', args.iso,
         '--output', out/'census'], out, 'census')
    census = json.loads((out/'census/census.json').read_text())
    sys.path.insert(0, str(ROOT/'tools'))
    from extract_native_animation import extract_clip
    sdk = snapshot/'sdk'
    header = (sdk/'src/melee/ft/native_fighter_runtime.h').read_text()
    definition = header.split('typedef struct NativeFighterDefinition {', 1)[1].split('} NativeFighterDefinition;', 1)[0]
    callbacks = sorted(set(re.findall(r'\(\*(\w+)\)', definition)+re.findall(
        r'\b(?:HSD_GObjEvent|HSD_GObjPredicate|Fighter_ItemEvent)\s+(\w+)\s*;', definition)))
    results = {}
    for member, short, kind, folder, stem in (
            ('zelda', 'Zd', 'Zelda', 'ftZelda', 'zelda'),
            ('sheik', 'Sk', 'Seak', 'ftSeak', 'seak')):
        m = census['members'][member]
        directory = out/member
        directory.mkdir()
        source = (out/'census'/member/'candidate.fms').read_text()
        dat, top = extract_clip(args.iso, 'Pl'+short+'.dat', 'ftData'+kind)
        values = stage(source, dat, top, m['articles'], directory)
        run([snapshot/'bin/native_content_tool.exe', 'script-c', directory/'owned.fms',
             member+'_owned_probe', directory/'generated.c'], directory, 'generate')
        generated = (directory/'generated.c').read_text()
        count, schema = re.search(r'count != (\d+) \|\| schema != (0x[\da-fA-F]+)ULL', generated).groups()
        assert int(count) == len(values)
        (directory/'parameters.inc').write_text('static const uint32_t values[] = {'+','.join(
            'UINT32_C('+str(v)+')' for v in values)+'};\n#define SCHEMA '+schema+'ULL\n', newline='\n')
        (directory/'required_host.inc').write_text('\n'.join(
            'host.'+name+'=(__typeof__(host.'+name+'))unexpected;' for name in sorted(set(
                re.findall(r'host->(\w+) == 0', generated))))+'\n', newline='\n')
        (directory/'callbacks.inc').write_text(''.join('VISIT_CALLBACK('+name+');\n' for name in callbacks), newline='\n')
        includes = f'#include <melee/ft/kinds/{folder}/ft{stem}.h>\n'
        rows = []
        for row in m['motion_rows']:
            fields = row['fields']
            rows.append('{ '+', '.join(fields[:3])+', '+', '.join(
                'NULL' if field in ('NULL','0') else 'stock_slot' for field in fields[3:])+ ' },')
        (directory/'motion.inc').write_text(includes+'static MotionState stock_states[] = {\n'+
            '\n'.join(rows)+'\n};\n#define MEMBER_KIND Ft_Kind_'+kind+'\n#define ARTICLE_COUNT '+
            str(len(m['articles']))+'\n#define PART_COUNT '+str(m['parts'])+'\n#define ACTION_COUNT '+
            str(m['actions'])+'\n#define SHEIK '+str(int(member=='sheik'))+'\n', newline='\n')
        proposed_base = (out/'census'/member/f'ft{member}_clone.c').read_text()
        for suffix in ('c','h'):
            assert (out/'census'/member/f'ft{member}_clone.{suffix}').read_bytes()==(
                HERE/'bases'/f'ft{member}_clone.{suffix}').read_bytes(), 'Proposed base no longer matches actual census'
        metadata = proposed_base[proposed_base.index('static const NativeFighterBone'):proposed_base.index('const NativeFighterDefinition')]
        title = member.capitalize()
        (directory/'metadata.inc').write_text(metadata+'\n#define BASE_BONES ft'+title+'Clone_Bones\n'+
            '#define BASE_ARTICLES ft'+title+'Clone_Articles\n#define BASE_DEMOS ft'+title+'Clone_DemoMotionNames\n'+
            ('#define BASE_POSES ftSheikClone_Poses\n' if member=='sheik' else ''), newline='\n')
        originals = (sdk/f'src/melee/ft/kinds/{folder}/ft{stem}.c').read_text()
        names = ['OnLoad','OnDeath','LoadSpecialAttrs','OnItemPickup','OnItemDrop',
                 'OnItemVisible','OnItemInvisible','OnKnockbackEnter','OnKnockbackExit']
        if member=='sheik':
            names += ['UnkMotionStates4']
        (directory/'original.inc').write_text('\n\n'.join(body(originals, 'ft'+short+'_Init_'+n)
            for n in names)+'\n', newline='\n')
        (directory/'original_calls.inc').write_text('\n'.join(
            '#define Original'+n+' ft'+short+'_Init_'+n for n in names)+'\n', newline='\n')
        flags = [snapshot/'compiler/clang.exe', '-std=gnu11', '-O2', '-ffp-contract=off',
            '-fuse-ld=lld', '-fms-runtime-lib=dll', '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES',
            '-D_CRT_SECURE_NO_WARNINGS', '-D_DLL', '-D_MT', '-I'+str(sdk/'src'),
            '-I'+str(sdk/'extern/dolphin/include'), '-I'+str(directory)]
        compile_commands = [flags+['-shared', directory/'generated.c', '-o', directory/'module.dll'],
            flags+[HERE/'ownership.c', '-o', directory/'ownership.exe'],
            flags+['-fsyntax-only', out/'census'/member/f'ft{member}_clone.c']]
        batch = directory/'compile.cmd'
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+''.join(
            subprocess.list2cmdline(list(map(str,c)))+'\nif errorlevel 1 exit /b 1\n' for c in compile_commands), newline='\n')
        run(['cmd', '/d', '/c', batch], directory, 'compile')
        results[member] = json.loads(run([directory/'ownership.exe', directory/'module.dll'], directory, 'ownership'))
    manifest = json.loads((snapshot/'freeze.json').read_text())
    for name, expected in manifest.items():
        assert sha(snapshot/name)==expected, name
    for name, expected in inputs.items():
        assert sha(Path(name))==expected, name
    receipt = {'status':'PASS generated independent ownership and controlled original lifecycle; normal package blocked',
        'snapshot_sha256':sha(snapshot/'freeze.json'), 'results':results, 'commands':commands,
        'census_sha256':sha(out/'census/census.json'),
        'inputs':inputs,
        'outputs':{str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file()},
        'limits':['Real clone-stock remains rejected; independent module ownership is not normal imported package acceptance.',
                  'Original lifecycle bodies execute unchanged; shared presentation and copy boundaries are controlled.',
                  'Stock article registrations are compared to immutable own asset slots, not executed in the generated on_load.',
                  'Prior move/article matrices remain separately accepted dependencies; this probe does not rerun them or render/play.']}
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', newline='\n')
    print(json.dumps(results))


if __name__ == '__main__':
    main()
