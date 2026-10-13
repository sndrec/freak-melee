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
    args = parser.parse_args(); root = args.root.resolve(); sdk = args.sdk.resolve(); out = args.output.resolve()
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    dat, fighter_root = extract_clip(args.iso.resolve(), 'PlSk.dat', 'ftDataSeak')
    (out / 'PlSk.dat').write_bytes(dat)
    word = lambda offset: struct.unpack_from('>I', dat, 32 + offset)[0]
    attributes = word(fighter_root + 4); parameters = []; lines = []
    source = (HERE / 'vanish.fms').read_text()
    for line in source.splitlines():
        match = re.fullmatch(r'attribute (float|int) (\w+) (\d+)', line)
        if not match: lines.append(line); continue
        kind, name, offset = match.groups(); offset = int(offset); bits = word(attributes + offset)
        value = struct.unpack('>f' if kind == 'float' else '>i', struct.pack('>I', bits))[0]
        parameters.append({'name': name, 'type': kind, 'value': repr(value), 'bits': bits,
                           'offset': offset, 'data_offset': attributes + offset})
        lines.append('')
    (out / 'owned.fms').write_text('\n'.join(lines) + '\n')
    (out / 'fighter_parameters.fmp').write_text(''.join(f"{p['type']} {p['name']} = {p['value']}\n" for p in parameters))
    sources = {}; extracted = []
    groups = {
        'src/melee/ft/kinds/ftSeak/ftseakspecialhi.c': 'WHOLE',
        'src/melee/it/kinds/itseakvanish.c': 'WHOLE',
        'src/melee/ft/ftmovement.c': ['ftCommon_Fall', 'ftCommon_FallBasic', 'ftCommon_ClampSelfVelX', 'ftCommon_CalcSelfAccel_DeaccelAir', 'ftCommon_CalcSelfAccel_Deaccel', 'ftCommon_SetSelfMovementFromGroundedMovement', 'ftCommon_CalcSelfAccel_Drift', 'ftCommon_CalcSelfAccel_DriftFrom', 'ftCommon_CalcSelfAccel_AccelToVelClampedFrom'],
        'src/melee/ft/ftcommon.c': ['ftCommon_UpdateFacing', 'ftCommon_8007DA24', 'ftCommon_8007D60C'],
        'src/melee/lb/lbvector.c': ['lbVector_Len_xy_accurate', 'lbVector_AngleXY'],
        'src/melee/it/it_26B1.c': ['it_8026B3A8', 'it_8026BD24', 'it_8026BDB4'],
        'src/melee/it/it_2725.c': ['it_8027518C', 'it_802751D8'],
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
