"""Retail metadata inputs and complete frozen stock motion-table initializers."""
import argparse, hashlib, json, re, struct, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p = argparse.ArgumentParser()
    for name in ('root', 'baseline', 'iso', 'output'):
        p.add_argument('--' + name, required=True, type=Path)
    a = p.parse_args()
    root, sdk, out = a.root.resolve(), a.baseline.resolve() / 'sdk', a.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sys.path.insert(0, str(root / 'tools'))
    from fighter_animation_io import Dat, disc_file
    inputs, expected, tables, callbacks, externals = {}, [], [], set(), {}
    common_raw = disc_file(a.iso, 'PlCo.dat')
    common = Dat(common_raw)
    common_root = common.roots['ftLoadCommonData']
    (out / 'PlCo.dat').write_bytes(common_raw)
    inputs['PlCo.dat'] = hashlib.sha256(common_raw).hexdigest()
    for kind, name, short, colors in ((10, 'Popo', 'Pp', ('Nr', 'Gr', 'Or', 'Re')),
                                       (11, 'Nana', 'Nn', ('Nr', 'Ye', 'Aq', 'Wh'))):
        descriptor = common.u32(common.u32(common_root + 16) + kind * 4)
        semantic = common.u32(descriptor + 4)
        count = common.u32(descriptor + 8)
        omission = common.u32(common.u32(common_root + 20) + kind * 4)
        assert count == 49 and (not omission or common.u32(omission + 4) == 0)
        mapping = list(common.data[semantic:semantic + 54])
        assert len([x for x in mapping if x != 255]) == 40
        expected.append('static const unsigned char expected_%s_map[54]={%s};' %
                        (name.lower(), ','.join(map(str, mapping))))
        costume_rows = []
        for color in colors:
            filename = 'Pl' + short + color + '.dat'
            blob = disc_file(a.iso, filename)
            (out / filename).write_bytes(blob)
            inputs[filename] = hashlib.sha256(blob).hexdigest()
            suffix = '' if color == 'Nr' else color
            joint = 'Ply' + name + '5K' + suffix + '_Share_joint'
            matanim = 'Ply' + name + '5K' + suffix + '_Share_matanim_joint'
            dat = Dat(blob)
            assert joint in dat.roots and matanim in dat.roots
            costume_rows.append('{"%s","%s","%s"}' % (filename, joint, matanim))
        expected.append('static const char* expected_%s_costumes[4][3]={%s};' %
                        (name.lower(), ','.join(costume_rows)))
        for filename in ('Pl' + short + '.dat', 'Pl' + short + 'AJ.dat'):
            blob = disc_file(a.iso, filename)
            (out / filename).write_bytes(blob)
            inputs[filename] = hashlib.sha256(blob).hexdigest()
        fighter = Dat((out / ('Pl' + short + '.dat')).read_bytes())
        raw = (out / ('Pl' + short + '.dat')).read_bytes()
        table_offset = 32 + len(fighter.data) + len(fighter.relocs) * 4
        strings = table_offset + 8 * (fighter.root_count + fighter.ref_count)
        references = []
        for i in range(fighter.ref_count):
            link, label = struct.unpack_from('>II', raw, table_offset + 8 * (fighter.root_count + i))
            start = strings + label
            symbol = raw[start:raw.index(0, start)].decode('ascii')
            slots = []
            while link != 0xffffffff:
                assert link not in slots and len(slots) < 1024
                slots.append(link)
                link = fighter.u32(link)
            references.append({'symbol':symbol, 'slots':slots, 'terminator':'0xFFFFFFFF'})
        externals[name] = references
        wait = fighter.u32(fighter.roots['ftData' + name] + 12)
        clips = sum(fighter.u32(wait + i * 24 + 8) != 0 for i in range(321))
        expected.append('static const unsigned expected_%s_clips=%d;' % (name.lower(), clips))
        rel = 'src/melee/ft/kinds/ft%s/ft%s.c' % (name, name.lower())
        text = (sdk / rel).read_text(encoding='utf-8')
        symbol = ('ftPp' if name == 'Popo' else 'ftNn') + '_Init_MotionStateTable'
        start = text.index('MotionState ' + symbol)
        opening = text.index('{', start)
        depth, end = 1, opening + 1
        while depth:
            depth += (text[end] == '{') - (text[end] == '}')
            end += 1
        table = text[start:end + 1]
        assert table.endswith('};')
        callbacks.update(re.findall(r'\b(ft\w+(?:_Anim|_IASA|_Phys|_Coll|CameraBox))\b', table))
        tables.append(table)
    # Function identities are inert boundaries, never invoked by this metadata test.
    # Table entries/flags/animation IDs are the complete original initializers.
    callbacks.update(('ftPp_SpecialN_Enter', 'ftPp_SpecialAirN_Enter',
                      'ftPp_SpecialS_Enter', 'ftPp_SpecialAirS_Enter',
                      'ftPp_SpecialHi_Enter', 'ftPp_SpecialAirHi_Enter',
                      'ftPp_SpecialLw_Enter', 'ftPp_SpecialAirLw_Enter'))
    stubs = '\n'.join('void %s(HSD_GObj* g){(void)g;abort();}' % n for n in sorted(callbacks))
    (out / 'stock_tables.inc').write_text(stubs + '\n' + '\n'.join(tables) + '\n', encoding='utf-8')
    (out / 'expected.inc').write_text('\n'.join(expected) + '\n', encoding='utf-8')
    sources = {str((sdk / ('src/melee/ft/kinds/ft%s/ft%s.c' % (n, n.lower())))):
               sha(sdk / ('src/melee/ft/kinds/ft%s/ft%s.c' % (n, n.lower())))
               for n in ('Popo', 'Nana')}
    (out / 'inputs.json').write_text(json.dumps({'iso':str(a.iso.resolve()), 'archives':inputs,
                                                'stock_sources':sources, 'external_ref_chains':externals},
                                               indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
