"""Decode real Sheik DAT tuning and extract complete immutable callback bodies."""
import argparse, hashlib, json, re, struct, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent

def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*(?://[^\n]*\n\s*)?\{', text)
    if not match: raise ValueError(name)
    end = match.end(); depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}'); end += 1
    return text[match.start():end]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'sdk', 'iso', 'output'): parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument("--with-vanish", action="store_true")
    args = parser.parse_args(); root = args.root.resolve(); sdk = args.sdk.resolve(); out = args.output.resolve()
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    dat, fighter_root = extract_clip(args.iso.resolve(), 'PlSk.dat', 'ftDataSeak')
    (out / 'PlSk.dat').write_bytes(dat)
    word = lambda offset: struct.unpack_from('>I', dat, 32 + offset)[0]
    attributes = word(fighter_root + 4); parameters = []; lines = []
    source = (HERE / 'needles.fms').read_text()
    if args.with_vanish:
        def split(component):
            a = component.index('func '); b = component.index('move '); c = component.index('article ')
            return component[:a], component[a:b], component[b:c], component[c:]
        needle = split(source); vanish = split((HERE / 'vanish.fms').read_text())
        source = needle[0] + vanish[0].replace('base ftDataSeak 341 24\n', '') + needle[1] + vanish[1] + needle[2] + vanish[2] + needle[3] + vanish[3]
    (out / 'candidate.fms').write_text(source)
    article = None; article_table = word(fighter_root + 72)
    slots = {'It_Kind_Seak_NeedleThrow': 0, 'It_Kind_Seak_NeedleHeld': 1}
    for line in source.splitlines():
        if line.startswith('article '): article = line.split()[1]
        inline = re.fullmatch(r'parameter (float|int|bool) (\w+) = (.+)', line)
        if inline:
            kind, name, value = inline.groups()
            bits = struct.unpack('>I', struct.pack('>f', float(value)))[0] if kind == 'float' else int(value) & 0xffffffff
            parameters.append({'name': name, 'type': kind, 'value': value, 'bits': bits,
                               'offset': None, 'data_offset': None, 'article_slot': None, 'source': 'owned source constant'})
            lines.append(''); continue
        match = re.fullmatch(r'(\s*)attribute (float|int) (\w+) (\d+)', line)
        if not match:
            lines.append(line.replace('item.attr.', 'item.params.')); continue
        indent, kind, name, offset = match.groups(); offset = int(offset)
        base = attributes if not indent else word(word(article_table + slots[article] * 4) + 4)
        bits = word(base + offset)
        value = struct.unpack('>f' if kind == 'float' else '>i', struct.pack('>I', bits))[0]
        parameters.append({'name': name if not indent else article + '.' + name, 'type': kind,
                           'value': repr(value), 'bits': bits, 'offset': offset, 'data_offset': base + offset,
                           'article_slot': slots[article] if indent else None})
        lines.append('')
    (out / 'owned.fms').write_text('\n'.join(lines) + '\n')
    (out / 'fighter_parameters.fmp').write_text(''.join(f"{p['type']} {p['name']} = {p['value']}\n" for p in parameters))
    sources = {}; extracted = []
    groups = {
        'src/melee/ft/kinds/ftSeak/ftseakspecialn.c': 'WHOLE',
        'src/melee/it/kinds/itseakneedleheld.c': 'WHOLE',
        'src/melee/it/kinds/itseakneedlethrown.c': 'WHOLE',
        'src/melee/ft/kinds/ftSeak/ftseak.c': ['ftSk_Init_80110198'],
        'src/melee/ft/kinds/ftSeak/ftseakspecials.c': ['ftSk_SpecialS_80111F70', 'ftSk_SpecialS_80111FA0'],
        'src/melee/ft/ftmovement.c': ['ftCommon_Fall', 'ftCommon_FallBasic', 'ftCommon_ClampSelfVelX', 'ftCommon_CalcSelfAccel_DeaccelAir', 'ftCommon_CalcSelfAccel_Deaccel', 'ftCommon_CalcSelfAccel_Drift', 'ftCommon_CalcSelfAccel_DriftFrom', 'ftCommon_CalcSelfAccel_AccelToVelClampedFrom'],
        'src/melee/it/it_26B1.c': ['it_8026B3A8', 'it_8026BD6C'],
        'src/melee/it/it_2725.c': ['it_80273130', 'it_80275158', 'it_80272940', 'it_80272560'],
        'src/melee/ft/ft_084E.c': ['ft_80084EEC'],
        'src/melee/it/item.c': ['Item_8026AB54'],
        'src/melee/it/itmaplib.c': ['it_802762BC'],
        'src/melee/lb/lbvector.c': ['lbVector_Mirror'],
    }
    for path, names in groups.items():
        raw = (sdk / path).read_bytes(); text = raw.decode(); entry = {'sha256': hashlib.sha256(raw).hexdigest()}
        if names == 'WHOLE':
            extracted.append(re.sub(r'(?m)^#include[^\n]*\n', '', text)); entry['scope'] = 'complete file; includes removed mechanically'
        else:
            entry['bodies'] = {}
            for name in names:
                value = body(text, name)
                extracted.append(('#define SOLUTION 0\n' + value + '\n#undef SOLUTION') if name == 'ftCommon_CalcSelfAccel_DeaccelAir' else value)
                entry['bodies'][name] = {'sha256': hashlib.sha256(value.encode()).hexdigest(), 'line': text[:text.index(value)].count('\n') + 1}
        sources[path] = entry
    (out / 'actual.inc').write_text('\n\n'.join(extracted) + '\n')
    (out / 'provenance.json').write_text(json.dumps({'DAT_sha256': hashlib.sha256(dat).hexdigest(),
        'parameters': parameters, 'sources': sources,
        'method': 'Independent big-endian retail DAT decoder; registered Sheik clone/import remains pending.'}, indent=2) + '\n')
    print('Decoded', len(parameters), 'retail attributes; extracted', len(groups), 'source files')

if __name__ == '__main__': main()
