"""Stage real DAT tuning and an unregistered complete Belay/Rope candidate."""
import argparse
import hashlib
import importlib.util
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PAIRED = HERE.parent / 'ice_climbers_linked'
TUNING = dict(rope_recruit_range=0x7c, rope_facing_threshold=0x80,
    rope_horizontal_divisor=0x84, rope_vertical_divisor=0x88,
    rope_start_gravity=0x8c, rope_start_terminal=0x90,
    rope_launch_speed=0x94, rope_distance_divisor=0x98,
    rope_launch_gravity=0x9c, rope_launch_terminal=0xa0,
    rope_drift_multiplier=0xb0, rope_drift_max_multiplier=0xb4)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'baseline', 'output', 'iso'):
        parser.add_argument('--' + name, type=Path, required=True)
    x = parser.parse_args()
    root, baseline, out = x.root.resolve(), x.baseline.resolve(), x.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    spec = importlib.util.spec_from_file_location('accepted_paired', PAIRED / 'paired_fixture.py')
    paired = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(paired)
    spec = importlib.util.spec_from_file_location('accepted_ice', HERE.parent / 'ice_climbers_composition/fixture.py')
    accepted = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(accepted)
    scalar = {'__file__': str(PAIRED / 'fixture.py'), '__name__': 'scalar_import_facts'}
    exec(compile((PAIRED / 'fixture.py').read_text(encoding='utf-8'), str(PAIRED / 'fixture.py'), 'exec'), scalar)
    dat, symbol = extract_clip(x.iso.resolve(), 'PlPp.dat', 'ftDataPopo')
    word = lambda at: struct.unpack_from('>I', dat, 32 + at)[0]
    attrs = word(symbol + 4)
    items = word(symbol + 72)
    article_attrs = [word(word(items + i * 4) + 4) for i in range(3)]
    imports = dict(accepted.FIGHTER_TUNING, **scalar['TUNING'], **paired.PRIMARY, **TUNING)
    article_imports = {'It_Kind_IceClimber_Ice': (accepted.ICE_TUNING, article_attrs[0]),
        'It_Kind_IceClimber_Blizzard': (accepted.BLIZZARD_TUNING, article_attrs[1]),
        'It_Kind_IceClimber_GumStrings': ({}, article_attrs[2])}
    accepted_source = paired.compose('popo')
    article_start = re.search(r'(?m)^article ', accepted_source).start()
    source = accepted_source[:article_start] + '\n' + (HERE / 'primary.fms').read_text(encoding='utf-8') + '\n' + accepted_source[article_start:] + '\n' + (HERE / 'rope.fms').read_text(encoding='utf-8')
    states = re.findall(r'(?m)^state (?:int|float|bool) .+$', source)
    source = re.sub(r'(?m)^state (?:int|float|bool) .+\n', '', source)
    source = source.replace('base ftDataPopo 341 26\n', 'base ftDataPopo 341 26\n' + '\n'.join(states) + '\n', 1)
    lines, parameters, scope, seen = [], [], None, set()
    for line in source.splitlines():
        if line.startswith('article '):
            scope = line.split()[1]
        if line.startswith('move ') or line.startswith('func ') or line.startswith('export func '):
            scope = None
        declaration = re.fullmatch(r'(\s*)(parameter|attribute) (float|int|bool) (\w+) (?:= )?(.+)', line)
        if not declaration:
            lines.append(line)
            continue
        indent, form, kind, name, default = declaration.groups()
        owner = scope if indent else None
        qualified = owner + '.' + name if owner else name
        if qualified in seen:
            raise ValueError('Duplicate parameter ' + qualified)
        seen.add(qualified)
        offset = int(default, 0) if form == 'attribute' else imports.get(name) if owner is None else article_imports[owner][0].get(name)
        origin = attrs if owner is None else article_imports[owner][1]
        if offset is None:
            value = int(default, 0) if kind == 'int' else float(default)
            bits = struct.unpack('<I', struct.pack('<i' if kind == 'int' else '<f', value))[0]
        else:
            bits = word(origin + offset)
            value = struct.unpack('>i' if kind == 'int' else '>f', struct.pack('>I', bits))[0]
        parameters.append(dict(name=qualified, type=kind, value=value, bits=bits,
            source_offset=offset, article_slot={'It_Kind_IceClimber_Ice': 0, 'It_Kind_IceClimber_Blizzard': 1, 'It_Kind_IceClimber_GumStrings': 2}.get(owner)))
        lines.append('')
    (out / 'owned.fms').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    (out / 'fighter_parameters.fmp').write_text(''.join(f"{p['type']} {p['name']} = {p['value']}\n" for p in parameters), encoding='utf-8')
    (out / 'PlPp.dat').write_bytes(dat)
    sources = [root / 'src/melee/ft/kinds/ftPopo/ftpopospecialhi.c',
        root / 'src/melee/it/kinds/itclimbersstring.c', root / 'src/melee/it/kinds/itlinkhookshot.c',
        root / 'src/melee/it/kinds/inlines.h', root / 'src/melee/lb/lbvector.c',
        root / 'src/melee/it/itCharItems.h', root / 'src/placeholder.h']
    (out / 'preparation.json').write_text(json.dumps(dict(parameters=parameters,
        DAT_sha256=hashlib.sha256(dat).hexdigest(), article_count=word(article_attrs[2]),
        sdk_manifest_sha256=sha(baseline / 'freeze.json'),
        sources={str(p.relative_to(root)): sha(p) for p in sources},
        status='Unregistered candidate; source comparison pending'), indent=2) + '\n', encoding='utf-8')
    print(out)

if __name__ == '__main__':
    main()
