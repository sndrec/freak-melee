"""Stage the paired source candidate and complete original bodies; no game."""
import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ACCEPTED = HERE.parent / 'ice_climbers_composition'

# These are fixture import facts, never runtime DAT-offset APIs.
PRIMARY = {
    'linked_squall_ground_launch': 0x28, 'linked_squall_air_launch': 0x2c,
    'linked_squall_solo_lift': 0x20, 'linked_squall_pair_lift': 0x24,
    'linked_squall_ground_max': 0x38, 'linked_squall_slope_accel': 0x6c,
    'linked_squall_fall_lag': 0x70,
}
SECONDARY = {
    'linked_squall_radius': 0xd0, 'linked_squall_fall_lag': 0x12c,
    'linked_belay_mobility': 0x130, 'linked_belay_landing_lag': 0x134,
    'linked_belay_input_threshold': 0x138, 'linked_belay_launch_speed': 0x13c,
    'linked_belay_launch_angle': 0x140, 'linked_belay_gravity': 0x144,
    'linked_belay_terminal': 0x148, 'linked_belay_drift_multiplier': 0xb0,
    'linked_belay_speed_multiplier': 0xb4, 'linked_belay_wall_restitution': 0x14c,
}

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' +
        re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match:
        raise ValueError('Missing complete body: ' + name)
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]

def names(text):
    return re.findall(r'(?m)^(?:static\s+)?(?:inline\s+)?(?:void|bool|float|f32)\s+(\w+)\s*\([^;{}]*\)\s*\{', text)

def compose(form):
    common = ['paired_exports', 'paired_hitlag']
    if form == 'popo':
        source = (ACCEPTED / 'fighter.fms').read_text(encoding='utf-8')
        source += '\n' + (HERE / 'solo.fms').read_text(encoding='utf-8')
        components = common + ['primary_cleanup', 'primary_squall']
    else:
        source = 'base ftDataNana 341 26\n'
        components = common + ['secondary_squall', 'secondary_belay']
    source += '\n' + '\n'.join((HERE / (name + '.fms')).read_text(encoding='utf-8') for name in components)
    # One actual base declaration; accepted component bytes stay untouched.
    source = re.sub(r'(?m)^base .+\n', '', source)
    state_lines = re.findall(r'(?m)^state (?:int|float|bool) .+$', source)
    source = re.sub(r'(?m)^state (?:int|float|bool) .+\n', '', source)
    source = 'base ' + ('ftDataPopo' if form == 'popo' else 'ftDataNana') + ' 341 26\n' + '\n'.join(state_lines) + '\n' + source
    if form == 'popo':
        source += '\n' + '\n'.join((ACCEPTED / (name + '.fms')).read_text(encoding='utf-8') for name in ('ice', 'blizzard'))
    return source

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('root', 'baseline', 'reference', 'output', 'iso'):
        parser.add_argument('--' + key, type=Path, required=True)
    x = parser.parse_args()
    root, sdk, reference, out = x.root.resolve(), x.baseline.resolve() / 'sdk', x.reference.resolve() / 'sdk', x.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    # Import the accepted offset catalogs directly, without rewriting them.
    sys.path.insert(0, str(ACCEPTED))
    import fixture as accepted
    scalar = {'__file__': str(HERE / 'fixture.py'), '__name__': 'paired_scalar_import_facts'}
    exec(compile((HERE / 'fixture.py').read_text(encoding='utf-8'), str(HERE / 'fixture.py'), 'exec'), scalar)
    manifests = {}
    for form, archive, symbol in [('popo', 'PlPp.dat', 'ftDataPopo'), ('nana', 'PlNn.dat', 'ftDataNana')]:
        target = out / form
        target.mkdir(exist_ok=True)
        dat, offset = extract_clip(x.iso.resolve(), archive, symbol)
        word = lambda at: struct.unpack_from('>I', dat, 32 + at)[0]
        attributes = word(offset + 4)
        item_table = word(offset + 72)
        article_attrs = [word(word(item_table + i * 4) + 4) for i in range(3)]
        imports = dict(accepted.FIGHTER_TUNING, **scalar['TUNING'], **(PRIMARY if form == 'popo' else SECONDARY))
        article_imports = {'It_Kind_IceClimber_Ice': (accepted.ICE_TUNING, article_attrs[0]),
                          'It_Kind_IceClimber_Blizzard': (accepted.BLIZZARD_TUNING, article_attrs[1])}
        lines, parameters, initializers, scope, declared = [], [], [], None, set()
        for line in compose(form).splitlines():
            if line.startswith('article '):
                scope = line.split()[1]
            match = re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)', line)
            if not match:
                lines.append(line)
                continue
            indent, kind, name, default = match.groups()
            owner = scope if indent else None
            qualified = (owner + '.' if owner else '') + name
            if qualified in declared:
                raise ValueError('Duplicate named parameter: ' + qualified)
            declared.add(qualified)
            at = imports.get(name) if owner is None else article_imports[owner][0].get(name)
            origin = attributes if owner is None else article_imports[owner][1]
            if at is None:
                value = int(default, 0) if kind == 'int' else float(default)
                bits = struct.unpack('<I', struct.pack('<i' if kind == 'int' else '<f', value))[0]
            else:
                bits = word(origin + at)
                value = struct.unpack('>i' if kind == 'int' else '>f', struct.pack('>I', bits))[0]
                if owner is None:
                    initializers.append('{uint32_t bits=UINT32_C(%d);memcpy((char*)&attributes[%d]+%d,&bits,4);}' % (bits, 0 if form == 'popo' else 1, at))
            parameters.append(dict(name=qualified, type=kind, value=value, bits=bits,
                                   attribute_offset=at, data_offset=origin + at if at is not None else None))
            lines.append('')
        (target / 'owned.fms').write_text('\n'.join(lines) + '\n', encoding='utf-8')
        (target / 'fighter_parameters.fmp').write_text(''.join(f"{p['type']} {p['name']} = {p['value']}\n" for p in parameters), encoding='utf-8')
        (target / archive).write_bytes(dat)
        (target / 'attributes.inc').write_text('\n'.join(initializers) + '\n', encoding='utf-8')
        manifests[form] = dict(DAT_sha256=hashlib.sha256(dat).hexdigest(), parameters=parameters)

    groups = {
        'src/melee/ft/kinds/ftPopo/ftpopospecials.c': lambda n: n not in ['ftPp_SpecialS_8011F720', 'inline2', 'inline3', 'inline4'] and not n.endswith('_Coll'),
        'src/melee/ft/kinds/ftNana/ftnanaspecials.c': lambda n: True,
        'src/melee/ft/kinds/ftNana/ftnanaspecialhi.c': lambda n: True,
        'src/melee/ft/kinds/ftNana/ftnana.c': lambda n: n in ['ftNn_Init_80122FAC', 'ftNn_Init_8012300C', 'ftNn_Init_8012309C'],
        'src/melee/ft/ftmovement.c': lambda n: n in ['ftCommon_Fall', 'ftCommon_FallBasic', 'ftCommon_CalcSelfAccel_DeaccelAir',
            'ftCommon_CalcSelfAccel_DriftSimple', 'ftCommon_CalcSelfAccel_AccelToVel', 'ftCommon_CalcGroundAccel_AccelToVel',
            'ftCommon_CalcSelfAccel_AccelToVelClamped', 'ftCommon_CalcSelfAccel_AccelToVelClampedFrom', 'ftCommon_CalcSelfAccel_Deaccel',
            'ftCommon_CalcGroundAccel_Deaccel', 'ftCommon_ClampGroundVel', 'ftCommon_ClampSelfVelX',
            'ftCommon_SetSelfMovementFromGroundedMovement_NoFriction'],
        'src/melee/ft/ft_081B.c': lambda n: n == 'ft_800849EC',
        'src/melee/ft/fighter.c': lambda n: n == 'Fighter_UnkSetFlag_8006CFBC',
        'src/melee/mp/mpcoll.c': lambda n: n == 'mpCopyCollData',
    }
    extracted, sources = [], {}
    for path, wanted in groups.items():
        raw = (reference / path).read_bytes()
        text = raw.decode('utf-8')
        entries = {}
        for name in names(text):
            if not wanted(name):
                continue
            original = body(text, name)
            extracted.append(original)
            entries[name] = dict(line=text[:text.index(original)].count('\n') + 1,
                                 sha256=hashlib.sha256(original.encode()).hexdigest())
        sources[path] = dict(sha256=hashlib.sha256(raw).hexdigest(), bodies=entries)
    # Original functions retain names and complete bodies. Declarations are
    # mechanical signatures, not behavioral substitutes.
    declarations = [original[:original.index('{')].strip() + ';' for original in extracted]
    box_text = (reference / 'src/melee/ft/kinds/ftNana/ftnanaspecials.c').read_text(encoding='utf-8')
    box = re.search(r'(?s)ftCollisionBox ftNn_Unk2_803CDD60 = \{.*?\};', box_text).group()
    (out / 'actual.inc').write_text('\n'.join(declarations) + '\n' + box + '\n\n' + '\n\n'.join(extracted) + '\n', encoding='utf-8')
    path = 'src/melee/ft/native_fighter_target.c'
    text = (sdk / path).read_text(encoding='utf-8')
    candidate = body(text, 'nativeFighterTarget_BonePosition')
    (out / 'candidate_boundaries.inc').write_text(candidate + '\n', encoding='utf-8')
    sources['candidate:' + path] = dict(sha256=sha(sdk / path), bodies={
        'nativeFighterTarget_BonePosition': dict(line=text[:text.index(candidate)].count('\n') + 1,
            sha256=hashlib.sha256(candidate.encode()).hexdigest())})
    (out / 'provenance.json').write_text(json.dumps(dict(forms=manifests, sources=sources,
        reference=str(reference), candidate_SDK=str(sdk), scope='Paired Squall entries/animation/ground physics; secondary Squall and Belay; no rope solver or private Squall effect acceptance'), indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
