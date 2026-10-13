"""Freeze complete original bodies and import named staging values from retail DAT."""
import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

def body(text, name):
    match = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', text)
    if not match:
        raise ValueError(name)
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]

FIGHTER_TUNING = {'ice_air_boost': 4, 'ice_landing_lag': 8,
    'ice_spawn_x': 12, 'ice_spawn_y': 16, 'blizzard_interval': 184,
    'blizzard_spawn_x': 188, 'blizzard_spawn_y': 192}
ICE_TUNING = {'lifetime': 0, 'bounce_lifetime_loss': 4,
    'shield_expiry_threshold': 8, 'impact_speed_limit': 12, 'launch_speed': 16,
    'slope_scale': 20, 'slope_with_velocity': 24, 'slope_against_velocity': 28,
    'stop_speed': 36, 'shrink_factor': 40, 'base_damage': 44,
    'speed_damage_multiplier': 48}
BLIZZARD_TUNING = {'lifetime': 0, 'speed': 4, 'vertical_acceleration': 8,
    'angle_min': 12, 'angle_max': 16}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('root', 'baseline', 'output', 'iso'):
        parser.add_argument('--' + key, type=Path, required=True)
    parser.add_argument('--form', choices=('popo', 'nana'), default='popo')
    args = parser.parse_args()
    root, base, out = args.root.resolve(), args.baseline.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sdk = base / 'sdk'
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    archive_name, symbol = ('PlPp.dat', 'ftDataPopo') if args.form == 'popo' else ('PlNn.dat', 'ftDataNana')
    dat, data_root = extract_clip(args.iso.resolve(), archive_name, symbol)
    (out / archive_name).write_bytes(dat)
    word = lambda offset: struct.unpack_from('>I', dat, 32 + offset)[0]
    attributes = word(data_root + 4)
    item_table = word(data_root + 72)
    article_roots = [word(item_table + i * 4) for i in range(3)]
    article_attrs = [word(offset + 4) for offset in article_roots]
    tuning = [(None, FIGHTER_TUNING, attributes),
        ('It_Kind_IceClimber_Ice', ICE_TUNING, article_attrs[0]),
        ('It_Kind_IceClimber_Blizzard', BLIZZARD_TUNING, article_attrs[1])]
    imported = {}
    for scope, offsets, start in tuning:
        for field, offset in offsets.items():
            kind = 'int' if field in ('base_damage', 'speed_damage_multiplier') else 'float'
            bits = word(start + offset)
            value = struct.unpack('>i' if kind == 'int' else '>f', struct.pack('>I', bits))[0]
            imported[(scope, field)] = dict(type=kind, bits=bits, value=value,
                offset=offset, data_offset=start + offset, scope=scope, name=field)
    source = '\n'.join((HERE / (n + '.fms')).read_text(encoding='utf-8') for n in ('fighter', 'ice', 'blizzard'))
    scope, lines, parameters = None, [], []
    for line in source.splitlines():
        if line.startswith('article '):
            scope = line.split()[1]
        match = re.fullmatch(r'(\s*)parameter (float|int|bool) (\w+) = (.+)', line)
        if not match:
            lines.append(line.replace('base ftDataPopo', 'base ' + symbol))
            continue
        indent, kind, field, value = match.groups()
        key = (scope if indent else None, field)
        entry = imported.get(key)
        if entry is None:
            if args.form == 'nana' and field in ('ice_launch_voice', 'blizzard_voice', 'blizzard_loop'):
                value = str({'ice_launch_voice': 130090, 'blizzard_voice': 0x1FC0F, 'blizzard_loop': 0x1FBEE}[field])
            val = int(value, 0) if kind == 'int' else float(value)
            bits = struct.unpack('<I', struct.pack('<i' if kind == 'int' else '<f', val))[0]
            entry = dict(type=kind, bits=bits, value=val, offset=None,
                data_offset=None, scope=key[0], name=field)
        entry = dict(entry)
        entry['qualified_name'] = (key[0] + '.' if key[0] else '') + field
        parameters.append(entry)
        lines.append('')
    (out / 'owned.fms').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    (out / 'fighter_parameters.fmp').write_text(''.join(
        f"{p['type']} {p['qualified_name']} = {p['value']}\n" for p in parameters), encoding='utf-8')
    attr_init = []
    for p in parameters:
        if p['offset'] is None:
            continue
        target = 'fighter_attrs' if p['scope'] is None else ('ice_attrs' if p['scope'].endswith('_Ice') else 'blizzard_attrs')
        attr_init.append('{ uint32_t bits=UINT32_C(%d); memcpy((uint8_t*)&%s+%d,&bits,4); }' % (p['bits'], target, p['offset']))
    (out / 'attributes.inc').write_text('\n'.join(attr_init) + '\n', encoding='utf-8')
    groups = {
        'src/melee/ft/kinds/ftPopo/ftpopospecialn.c': [
            'ftPp_SpecialN_Enter', 'ftPp_SpecialAirN_Enter', 'ftPp_SpecialN_Anim', 'ftPp_SpecialAirN_Anim',
            'ftPp_SpecialN_IASA', 'ftPp_SpecialAirN_IASA', 'ftPp_SpecialN_Phys', 'ftPp_SpecialAirN_Phys',
            'ftPp_SpecialN_Coll', 'ftPp_SpecialAirN_Coll', 'inlineA0', 'ftPp_SpecialN_8011F500'],
        'src/melee/ft/kinds/ftPopo/ftpopospeciallw.c': [
            'ftPp_SpecialHi_80122898', 'ftPp_SpecialLw_Enter', 'ftPp_SpecialAirLw_Enter',
            'ftPp_SpecialLw_Anim', 'ftPp_SpecialAirLw_Anim', 'ftPp_SpecialLw_IASA', 'ftPp_SpecialAirLw_IASA',
            'ftPp_SpecialLw_Phys', 'ftPp_SpecialAirLw_Phys', 'ftPp_set_cbs', 'ftPp_SpecialLw_Coll_inline',
            'fn_80122B54', 'ftPp_SpecialLw_Coll_Land', 'ftPp_SpecialLw_Coll', 'ftPp_SpecialAirLw_Coll', 'fn_80122D2C'],
        'src/melee/ft/kinds/ftPopo/ftpopo.c': ['ftPp_Init_8011F16C', 'ftPp_Init_8011F190', 'ftPp_Init_OnDeath'],
        'src/melee/it/kinds/itclimbersice.c': [
            'itClimbersIce_sub_x4', 'itClimbersice_Spawn', 'itClimbersice_Spawn2', 'it_802C1590',
            'it_802C16F8', 'it_802C17DC', 'it_2725_Logic90_Destroyed', 'it_802C1854', 'it_802C1950',
            'itClimbersice_UnkMotion0_Anim', 'itClimbersice_UnkMotion0_Phys', 'itClimbersice_UnkMotion0_Coll',
            'it_802C1A58', 'itClimbersice_UnkMotion1_Anim', 'itClimbersice_UnkMotion1_Phys',
            'itClimbersice_UnkMotion1_Coll', 'it_802C1AE4', 'itClimbersice_UnkMotion2_Anim',
            'itClimbersice_Phys_inline', 'itClimbersice_UnkMotion2_Phys', 'itClimbersice_Coll',
            'itClimbersice_UnkMotion2_Coll', 'fn_802C1D44', 'itClimbersice_UnkMotion3_Anim',
            'itClimbersice_UnkMotion3_Phys', 'itClimbersice_UnkMotion3_Coll',
            'itClimbersIce_Logic90_DmgDealt', 'itClimbersIce_Logic90_Reflected', 'itClimbersIce_Logic90_Clanked',
            'it_2725_Logic90_HitShield', 'itClimbersIce_Logic90_Absorbed', 'itClimbersIce_Logic90_ShieldBounced',
            'itClimbersIce_Logic90_EvtUnk'],
        'src/melee/it/kinds/itclimbersblizzard.c': [
            'spawn_item_0z', 'itClimbersBlizzard_Spawn', 'itClimbersBlizzard_802C2248',
            'itClimbersBlizzard_802C2358', 'itClimbersBlizzard_UnkMotion0_Anim',
            'itClimbersBlizzard_UnkMotion0_Phys', 'itClimbersBlizzard_UnkMotion0_Coll',
            'itClimbersBlizzard_DmgDealt', 'itClimbersBlizzard_Reflected', 'itClimbersBlizzard_Clanked',
            'itClimbersBlizzard_HitShield', 'itClimbersBlizzard_Absorbed', 'itClimbersBlizzard_ShieldBounced',
            'itClimbersBlizzard_EvtUnk'],
    }
    includes, provenance = [], {}
    for path, names in groups.items():
        raw = (sdk / path).read_bytes()
        text = raw.decode('utf-8')
        entry = {'sha256': hashlib.sha256(raw).hexdigest(), 'bodies': {}}
        for name in names:
            actual = body(text, name)
            includes.append(actual)
            entry['bodies'][name] = {'sha256': hashlib.sha256(actual.encode()).hexdigest(),
                'line': text[:text.index(actual)].count('\n') + 1}
        provenance[path] = entry
    (out / 'actual.inc').write_text('\n\n'.join(includes) + '\n', encoding='utf-8')
    shared = {
        'src/melee/it/native_article.c': ['article_logic', 'nativeArticle_Create'],
        'src/melee/it/it_2725.c': ['it_80272860', 'it_80272980', 'it_80272F7C', 'it_80273030', 'it_80275158'],
        'src/melee/it/itmaplib.c': ['it_80276308', 'it_802763E0', 'it_8027770C'],
    }
    shared_text = []
    for path, names in shared.items():
        raw = (sdk / path).read_bytes()
        text = raw.decode('utf-8')
        entry = {'sha256': hashlib.sha256(raw).hexdigest(), 'bodies': {}}
        for name in names:
            actual = body(text, name)
            shared_text.append(actual)
            entry['bodies'][name] = {'sha256': hashlib.sha256(actual.encode()).hexdigest(),
                'line': text[:text.index(actual)].count('\n') + 1}
        provenance[path] = entry
    (out / 'shared.inc').write_text('\n\n'.join(shared_text) + '\n', encoding='utf-8')
    (out / 'provenance.json').write_text(json.dumps(dict(form=args.form,
        archive=archive_name, DAT_sha256=hashlib.sha256(dat).hexdigest(), parameters=parameters,
        article_roots=article_roots, sources=provenance,
        import_scope='Independent read-only big-endian retail staging import; no registered IC clone/import or paired package.'), indent=2) + '\n', encoding='utf-8')
    print(f'Frozen {sum(len(v) for v in groups.values())} complete original bodies; {len(parameters)} named parameters.')

if __name__ == '__main__':
    main()
