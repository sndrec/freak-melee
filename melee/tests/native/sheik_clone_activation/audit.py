"""Refresh owned Sheik retail metadata and source closure; no package activation."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def clean(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)


def split(text):
    result, depth, quote, begin = [], 0, None, 0
    for index, character in enumerate(text):
        if quote:
            if character == quote and (not index or text[index - 1] != '\\'):
                quote = None
        elif character in '\"\'':
            quote = character
        elif character in '{([':
            depth += 1
        elif character in '})]':
            depth -= 1
        elif character == ',' and not depth:
            if text[begin:index].strip():
                result.append(text[begin:index].strip())
            begin = index + 1
    if text[begin:].strip():
        result.append(text[begin:].strip())
    return result


def arrays(text, pattern):
    result = {}
    for match in re.finditer(pattern, clean(text)):
        start, depth = match.end(), 1
        stripped = clean(text)
        end = start
        while depth:
            depth += (stripped[end] == '{') - (stripped[end] == '}')
            end += 1
        result[match[1]] = split(stripped[start:end - 1])
    return result


def name_hash(name):
    result = 2166136261
    for byte in name.encode('ascii'):
        result = ((result ^ byte) * 16777619) & 0xffffffff
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'snapshot', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--iso', type=Path)
    args = parser.parse_args()
    root, snapshot, output = args.root.resolve(), args.snapshot.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    iso = args.iso or root.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso'
    sys.path.insert(0, str(root / 'tools'))
    from extract_native_animation import extract_clip
    word = lambda data, offset: struct.unpack_from('>I', data, 32 + offset)[0]
    co, co_root = extract_clip(iso, 'PlCo.dat', 'ftLoadCommonData')
    data, data_root = extract_clip(iso, 'PlSk.dat', 'ftDataSeak')
    part_table, omission_table = word(co, co_root + 16), word(co, co_root + 20)
    descriptor = word(co, part_table + 7 * 4)
    mapping, runtime_count = word(co, descriptor + 4), word(co, descriptor + 8)
    forward = (root / 'src/melee/ft/forward.h').read_text()
    roles = re.findall(r'(FtPart_\w+)\s*(?:,|=)', forward[forward.index('typedef enum Fighter_Part'):forward.index('} Fighter_Part;')])
    roles = roles[:53] + ['53']
    bones = [{'part': index, 'role': roles[index], 'joint': joint}
             for index, joint in enumerate(co[32 + mapping:32 + mapping + 54]) if joint != 255]
    omission = word(co, omission_table + 7 * 4)
    omissions = []
    if omission:
        records, count = word(co, omission), word(co, omission + 4)
        omissions = [list(co[32 + records + 4 * index:32 + records + 4 * index + 4]) for index in range(count)]
    costume_counts = []
    costumes = []
    for suffix in ('Nr', 'Re', 'Bu', 'Gr', 'Wh'):
        stem = 'Seak5K' if suffix == 'Nr' else 'Seak5K' + suffix
        archive, symbol = 'PlSk' + suffix + '.dat', 'Ply' + stem + '_Share_joint'
        costume, joint_root = extract_clip(iso, archive, symbol)
        material_symbol = 'Ply' + stem + '_Share_matanim_joint'
        material_data, material_root = extract_clip(iso, archive, material_symbol)
        assert material_data == costume
        visited = set()

        def visit(offset):
            if not offset:
                return
            assert offset not in visited
            visited.add(offset)
            visit(word(costume, offset + 8))
            visit(word(costume, offset + 12))

        visit(joint_root)
        assert len(visited) == runtime_count
        costume_counts.append(len(visited))
        costumes.append({'archive': archive, 'joint_symbol': symbol, 'joints': len(visited),
                        'material_symbol': material_symbol, 'material_root': material_root,
                        'sha256': hashlib.sha256(costume).hexdigest()})
    clone_path = root / 'src/melee/ft/kinds/ftSeak/ftsheik_clone.c'
    clone = clone_path.read_text()
    for bone in bones:
        assert re.search(r'\{\s*' + bone['role'] + r'\s*,\s*' + str(bone['joint']) + r'\s*\}', clone)
    assert len(bones) == 52 and runtime_count == 58 and not omissions
    pose_source = (HERE.parent / 'sheik_chain_composition/fighter.fms').read_text()
    poses = [{'name': name, 'name_hash': name_hash(name), 'slot': int(slot), 'path': path}
             for name, slot, path in re.findall(r'(?m)^pose_graph (\w+) slot (\d+) path "([^"]*)"', pose_source)]
    for pose in poses:
        assert f"UINT32_C({pose['name_hash']}), {pose['slot']}, \"{pose['path']}\"" in clone
    ftdata = (root / 'src/melee/ft/ftdata.c').read_text()
    tables = arrays(ftdata, r'\b(ftData_\w+|ftKindCalcIndiviParamTable)\s*\[[^\]]*\]\s*=\s*\{')
    callbacks = {name: rows[7] for name, rows in tables.items() if len(rows) == 33}
    assert split(callbacks['ftData_Table_Unk0'][1:-1]) == ['0', '317']
    assert split(callbacks['ftData_UnkIntPairs'][1:-1]) == ['0', '14']
    assert callbacks['ftData_UnkBytePerCharacter'] == '17'
    source = (root / 'src/melee/ft/kinds/ftSeak/ftseak.c').read_text()
    motions = arrays(source, r'\b(ftSk_Init_MotionStateTable)\s*\[[^\]]*\]\s*=\s*\{')['ftSk_Init_MotionStateTable']
    motion_rows = [{'id': 341 + index, 'fields': split(row[1:-1]),
                    'authored_component': 'Needle' if index < 8 else 'Chain' if index < 14 else 'Vanish' if index < 20 else 'Transform pending'}
                   for index, row in enumerate(motions)]
    item_table = word(data, data_root + 72)
    slots = [word(data, item_table + index * 4) for index in range(6)]
    assert all(slots) and len(motions) == 24
    sound_source = root / 'src/melee/lb/lbaudio_ax.static.h'
    sound = arrays(sound_source.read_text(), r'\b(lbl_803BB3C0)\s*\[[^\]]*\]\s*=\s*\{')['lbl_803BB3C0'][19]
    inputs = list((root / 'src/melee/ft/kinds/ftSeak').glob('*.[ch]'))
    inputs += [root / path for path in (
        'src/melee/ft/ftdata.c', 'src/melee/ft/ftcommon.c', 'src/melee/pl/player.c',
        'src/melee/gm/native_match_launch.c', 'src/melee/ft/native_fighter_runtime.c',
        'src/melee/ft/native_fighter_runtime.h', 'src/melee/ft/native_fighter_project.c',
        'src/melee/ft/native_fighter_article_bindings.c', 'src/melee/ft/ftcharacter_assets.c',
        'src/melee/ft/fighter.c', 'src/melee/ft/ftcolanim.c', 'src/melee/ft/ftcpuattack.c',
        'src/melee/ft/kinds/ftCommon/ftCo_0A01.c', 'src/melee/ft/kinds/ftZelda/ftzeldaspeciallw.c',
        'src/melee/it/it_3F2F.c', 'src/melee/it/it_26B1.c', 'src/melee/ef/efsync.c',
        'src/melee/ft/ftcharacter_asset_layout.h', 'src/melee/lb/lbaudio_ax.static.h')]
    report = {'status': 'PASS metadata source census; package/transform/independence unaccepted',
        'snapshot': str(snapshot), 'freeze_sha256': sha(snapshot / 'freeze.json'),
        'commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip(),
        'internal_kind': 7, 'character_kind': 19, 'root': 'ftDataSeak', 'actions': 317, 'demos': 14,
        'PlCo_sha256': hashlib.sha256(co).hexdigest(), 'PlSk_sha256': hashlib.sha256(data).hexdigest(),
        'bones': bones, 'omissions': omissions, 'runtime_parts': runtime_count, 'costumes': costumes,
        'pose_graphs': poses, 'raw_item_offsets': slots,
        'asset_layout': {'article_mask': 15, 'joint_graph_mask': 48, 'item_format': 'Chain'},
        'sound_source_record': sound, 'effect_bank': int(callbacks['ftData_UnkBytePerCharacter']),
        'callbacks': callbacks, 'motion_rows': motion_rows,
        'authored_phase_count': 80, 'pending_transform_phase_count': 16,
        'inputs': {str(path.relative_to(root)).replace('\\', '/'): sha(path) for path in sorted(set(inputs))}}
    (output / 'census.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'PASS {len(bones)} bone bindings / {runtime_count} parts / all {len(costumes)} costumes / {len(motions)} motion rows')
    print(output / 'census.json')


if __name__ == '__main__':
    main()
