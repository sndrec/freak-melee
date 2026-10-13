"""Freeze complete stock phase bodies and decode owned staging tuning from DAT."""
import argparse, hashlib, json, re, struct, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent

def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match: raise ValueError(name)
    end = match.end(); depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}'); end += 1
    return text[match.start():end]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'sdk', 'iso', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args(); root = args.root.resolve(); sdk = args.sdk.resolve(); out = args.output.resolve()
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    dat, fighter_root = extract_clip(args.iso.resolve(), 'PlZd.dat', 'ftDataZelda')
    (out / 'PlZd.dat').write_bytes(dat)
    word = lambda offset: struct.unpack_from('>I', dat, 32 + offset)[0]
    fighter_attributes = word(fighter_root + 4); article_table = word(fighter_root + 72)
    slots = {'It_Kind_Zelda_DinFire': 0, 'It_Kind_Zelda_DinFire_Explode': 1}
    source = (HERE / 'candidate.fms').read_text(); article = None; lines = []; provenance = []
    for line in source.splitlines():
        if line.startswith('article '): article = line.split()[1]
        match = re.fullmatch(r'(\s*)attribute (float|int|bool) (\w+) (\d+)', line)
        if not match:
            lines.append(line.replace('item.attr.', 'item.params.')); continue
        indent, kind, name, offset = match.groups(); offset = int(offset)
        base = fighter_attributes if not indent else word(word(article_table + slots[article] * 4) + 4)
        if kind == 'bool':
            bits = dat[32 + base + offset]; value = 'true' if bits else 'false'
        else:
            bits = word(base + offset)
            value = repr(struct.unpack('>f' if kind == 'float' else '>i', struct.pack('>I', bits))[0])
        lines.append('')
        provenance.append({'name': name if not indent else article + '.' + name, 'type': kind,
                           'value': value, 'bits': bits, 'offset': offset,
                           'data_offset': base + offset, 'article_slot': slots[article] if indent else None})
    (out / 'owned.fms').write_text('\n'.join(lines) + '\n')
    (out / 'fighter_parameters.fmp').write_text(''.join(f"{p['type']} {p['name']} = {p['value']}\n" for p in provenance))
    sources = {}; extracted = []
    groups = {
        'src/melee/ft/kinds/ftZelda/ftzeldaspecialn.c': 'WHOLE',
        'src/melee/ft/kinds/ftZelda/ftzeldaspeciallw.c': ['ftZd_SpecialLw_8013B540', 'ftZd_SpecialLw_8013B574', 'ftZd_SpecialLw_8013B5C4', 'ftZd_SpecialLw_8013B5EC'],
        'src/melee/it/kinds/itzeldadinfire.c': ['it_802C3D74', 'it_802C3AFC', 'itZeldadinfire_UnkMotion0_Anim_inline', 'itZeldadinfire_UnkMotion0_Anim', 'itZeldadinfire_UnkMotion1_Anim', 'itZeldadinfire_UnkMotion0_Phys', 'itZeldadinfire_UnkMotion1_Phys', 'itZeldadinfire_UnkMotion0_Coll', 'itZeldadinfire_UnkMotion1_Coll', 'itZeldaDinFire_Logic65_Reflected', 'itZeldaDinFire_Logic65_Destroyed', 'it_802C3D44', 'itZeldaDinFire_Logic65_Clanked', 'itZeldaDinFire_Logic65_Absorbed', 'itZeldaDinFire_Logic65_EvtUnk'],
        'src/melee/it/kinds/itzeldadinfireexplode.c': ['it_802C46C4', 'itZeldadinfireexplode_UnkMotion0_Anim', 'itZeldadinfireexplode_UnkMotion0_Phys', 'itZeldadinfireexplode_UnkMotion0_Coll', 'itZeldaDinFireExplode_Logic66_Destroyed', 'itZeldaDinFireExplode_Logic66_Clanked', 'itZeldaDinFireExplode_Logic66_Absorbed', 'itZeldaDinFireExplode_Logic66_ShieldBounced', 'itZeldaDinFireExplode_Logic66_HitShield', 'itZeldaDinFireExplode_Logic66_EvtUnk'],
        'src/melee/ft/ftmovement.c': ['ftCommon_Fall', 'ftCommon_CalcSelfAccel_DeaccelQuickAir'],
        'src/melee/it/it_2725.c': ['it_80273130', 'it_802751D8', 'it_80275158', 'it_80272940'],
        'src/melee/ft/kinds/ftZelda/ftzelda.c': ['ftZd_Init_OnDeath', 'ftZd_Init_OnItemPickup', 'ftZd_Init_OnItemDrop', 'ftZd_Init_OnItemVisible', 'ftZd_Init_OnItemInvisible', 'ftZd_Init_OnKnockbackEnter', 'ftZd_Init_OnKnockbackExit'],
    }
    for path, names in groups.items():
        raw = (sdk / path).read_bytes(); text = raw.decode(); entry = {'sha256': hashlib.sha256(raw).hexdigest()}
        if names == 'WHOLE':
            extracted.append(re.sub(r'(?m)^#include[^\n]*\n', '', text)); entry['scope'] = 'complete file with includes removed'
        else:
            entry['bodies'] = {}
            if path.endswith('itzeldadinfire.c'):
                typedef = text[text.index('typedef struct {'):text.index('} ItZeldaDinFire_ItemVars;') + len('} ItZeldaDinFire_ItemVars;')]
                extracted.append(typedef)
            for name in names:
                value = body(text, name); extracted.append(value)
                entry['bodies'][name] = {'sha256': hashlib.sha256(value.encode()).hexdigest(), 'line': text[:text.index(value)].count('\n') + 1}
        sources[path] = entry
    (out / 'actual.inc').write_text('\n\n'.join(extracted) + '\n')
    (out / 'provenance.json').write_text(json.dumps({'DAT_sha256': hashlib.sha256(dat).hexdigest(), 'parameters': provenance,
        'method': 'Independent big-endian DAT decoder; normal registered Zelda parameter import is pending.', 'sources': sources}, indent=2) + '\n')
    print('Decoded', len(provenance), 'actual retail staging parameters; preserved', len(groups), 'source files')

if __name__ == '__main__': main()
