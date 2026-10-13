"""Pin unchanged original Chain bodies and actual DAT tuning; no compilation."""
import argparse
import hashlib
import importlib.util
import json
import re
import struct
import sys
from pathlib import Path
from compose import compose

HERE = Path(__file__).resolve().parent
SOURCES = (
    'src/melee/ft/kinds/ftSeak/ftseakspecials.c',
    'src/melee/it/kinds/itseakchain.c',
    'src/melee/it/kinds/inlines.h',
    'src/melee/it/kinds/itlinkhookshot.c',
    'src/melee/ft/kinds/ftSeak/ftseak.c',
    'src/melee/it/it_26B1.c',
    'src/melee/it/it_2725.c',
    'src/melee/lb/lbvector.c',
    'src/melee/mp/mpcoll.c',
    'extern/dolphin/include/dolphin/mtx.h',
    'src/melee/ft/forward.h',
    'src/melee/ft/kinds/ftSeak/types.h',
    'src/melee/it/itCharItems.h',
    'src/melee/ft/native_fighter_module.c',
    'src/melee/ft/kinds/ftSeak/ftseakspecialn.c',
)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    snapshot = args.snapshot.resolve()
    sdk = snapshot / 'sdk'
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    dat, fighter_root = extract_clip(args.iso.resolve(), 'PlSk.dat', 'ftDataSeak')
    word = lambda at: struct.unpack_from('>I', dat, 32 + at)[0]
    special = word(fighter_root + 4)
    article_table = word(fighter_root + 72)
    slots = {'It_Kind_Seak_NeedleThrow': 0,
             'It_Kind_Seak_NeedleHeld': 1, 'It_Kind_Seak_Chain': 3}
    source = compose()
    (output / 'candidate.fms').write_text(source, newline='\n')
    article = None
    parameters = []
    stripped = []
    for line in source.splitlines():
        if line.startswith('article '):
            article = line.split()[1]
        literal = re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)', line)
        attribute = re.fullmatch(r'(\s*)attribute (float|int|byte) (\w+) (\d+)', line)
        if literal:
            indent, kind, name, value = literal.groups()
            bits = struct.unpack('>I', struct.pack('>f', float(value)))[0] if kind == 'float' else int(value) & 0xffffffff
            offset = None
        elif attribute:
            indent, kind, name, offset = attribute.groups()
            offset = int(offset)
            data_base = special if not indent else word(word(article_table + slots[article] * 4) + 4)
            bits = word(data_base + offset)
            value = repr(struct.unpack('>f' if kind == 'float' else '>i', struct.pack('>I', bits))[0])
        else:
            stripped.append(line.replace('item.attr.', 'item.params.'))
            continue
        qualified = name if not indent else article + '.' + name
        parameters.append({'name': qualified, 'type': kind, 'bits': bits,
                           'value': value, 'source_offset': offset,
                           'article_slot': slots[article] if indent else None})
        stripped.append('')
    (output / 'owned.fms').write_text('\n'.join(stripped).rstrip() + '\n', newline='\n')
    (output / 'fighter_parameters.fmp').write_text(''.join(
        f"{p['type']} {p['name']} = {p['value']}\n" for p in parameters), newline='\n')
    # These are copied untouched as extraction inputs, not rewritten expectations.
    originals = output / 'original_inputs'
    for relative in SOURCES:
        destination = originals / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes((sdk / relative).read_bytes())
    spec = importlib.util.spec_from_file_location('needle_extraction', HERE.parent / 'sheik_script_composition/needle_fixture.py')
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    extracted = {}
    for relative, names in {
        SOURCES[0]: 'WHOLE', SOURCES[1]: 'WHOLE',
        SOURCES[2]: ['Item_RetractChain'],
        SOURCES[3]: ['it_802A3C98', 'it_802A6DC8', 'it_802A4420', 'it_802A43EC', 'it_802A43B8'],
        SOURCES[4]: ['ftSk_Init_80110198'],
        SOURCES[5]: ['it_8026B724', 'it_8026B73C', 'it_8026B894', 'it_8026BB68'],
        SOURCES[6]: ['it_80272A3C'],
        SOURCES[7]: ['lbVector_Normalize'],
        SOURCES[13]: ['common_attribute'],
    }.items():
        text = (sdk / relative).read_text()
        content = re.sub(r'(?m)^#include[^\n]*\n', '', text) if names == 'WHOLE' else '\n\n'.join(helper.body(text, n) for n in names)
        destination = output / (Path(relative).stem + '_actual.inc')
        destination.write_text(content.rstrip() + '\n', newline='\n')
        extracted[relative] = {'file': destination.name, 'sha256': sha(destination), 'functions': names}
    status = {'status': 'PREPARED uncompiled source candidate; no comparison accepted',
              'snapshot': str(snapshot), 'freeze_sha256': sha(snapshot / 'freeze.json'),
              'parameters': parameters, 'source_inputs': {p: sha(sdk / p) for p in SOURCES},
              'candidate_components': {str(p.relative_to(root)): sha(p) for p in [HERE / 'fighter.fms', HERE / 'article.fms', HERE.parent / 'sheik_script_composition/needles.fms']},
              'extractions': extracted,
              'pending': ['Sheik copied-base rig/article/pose ownership; normal clone/package/render acceptance',
                          'Whole-fighter load/reset/Transform composition and independent activation']}
    (output / 'preparation.json').write_text(json.dumps(status, indent=2) + '\n')
    print(output / 'preparation.json')


if __name__ == '__main__':
    main()
