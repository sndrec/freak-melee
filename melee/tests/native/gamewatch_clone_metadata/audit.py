"""Read retail resources and freeze the inputs for an independent Workshop base.

This is an asset census, not execution or full-fighter parity acceptance.
"""
import argparse
import hashlib
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    snapshot, output = args.snapshot.resolve(), args.output.resolve()
    if output.exists() or snapshot in output.parents or REPO in output.parents:
        parser.error('use a fresh output outside the checkout and frozen SDK')
    manifest = json.loads((snapshot / 'freeze.json').read_text())
    sources = {}

    def source(name):
        path = snapshot / 'sdk' / name
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        key = str(path.relative_to(snapshot)).replace('/', '\\')
        if manifest[key] != digest:
            raise ValueError('frozen source hash mismatch: ' + name)
        sources[name] = digest
        return path.read_text(encoding='utf-8')

    # Use the frozen reader; no live compiler or embedded include paths.
    source('tools/fighter_animation_io.py')
    sys.path.insert(0, str(snapshot / 'sdk/tools'))
    from fighter_animation_io import Dat, disc_file, joints
    raw = {name: disc_file(args.iso, name)
           for name in ('PlCo.dat', 'PlGw.dat', 'PlGwNr.dat')}
    common, fighter, costume = (Dat(raw[name])
                               for name in ('PlCo.dat', 'PlGw.dat', 'PlGwNr.dat'))
    forward = source('src/melee/ft/forward.h')
    kinds = re.search(r'typedef enum FighterKind\s*\{(.*?)Ft_Kind_None',
                      forward, re.S)[1]
    kind = re.findall(r'\b(Ft_Kind_\w+)\s*,', kinds).index('Ft_Kind_GameWatch')
    assert kind == 24
    common_root = common.roots['ftLoadCommonData']
    parts = common.u32(common.u32(common_root + 16) + kind * 4)
    joint_map, part_map, count = (common.u32(parts + offset)
                                  for offset in (0, 4, 8))
    mapping = list(common.data[part_map:part_map + 54])
    reverse = list(common.data[joint_map:joint_map + count])
    omissions = common.u32(common.u32(common_root + 20) + kind * 4)
    graph = joints(costume, costume.roots['PlyGamewatch5K_Share_joint'])
    assert count == len(graph) == 53 and omissions == 0
    assert len(mapping) == 54 and sum(v != 255 for v in mapping) == 38
    for part, joint in enumerate(mapping):
        if joint != 255:
            assert joint < count and reverse[joint] == part
    assert list(costume.roots) == ['PlyGamewatch5K_Share_joint']

    stock = source('src/melee/ft/kinds/ftGameWatch/ftgamewatch.c')
    registrations = re.findall(
        r'it_8026B3F8\(items\[(\d+)\], (It_Kind_GameWatch_\w+)\)', stock)
    assert len(registrations) == 10
    # Check the live import map separately; the accepted SDK predates these rows.
    bindings_path = REPO / 'src/melee/ft/native_fighter_article_bindings.c'
    bindings = re.findall(
        r'SLOT\(Ft_Kind_GameWatch, (It_Kind_GameWatch_\w+), (\d+), (-?\d+)\)',
        bindings_path.read_text())
    assert bindings == [(name, index, '-1') for index, name in registrations]
    root = fighter.roots['ftDataGamewatch']
    table = fighter.u32(root + 72)
    articles = []
    for index, name in registrations:
        slot = int(index)
        descriptor = fighter.u32(table + slot * 4)
        model = fighter.u32(descriptor + 16)
        assert descriptor and model and fighter.u32(descriptor + 4)
        articles.append(dict(name=name, slot=slot, descriptor=descriptor,
                             tuning=fighter.u32(descriptor + 4),
                             states=fighter.u32(descriptor + 12),
                             model=model, physical_joints=len(joints(fighter, fighter.u32(model)))))
    visibility = fighter.u32(table + 40)
    assert visibility and 'fp->x5AC.xC[4] = items[10]' in stock
    data_source = source('src/melee/ft/ftdata.c')
    def source_count(table_name):
        rows = re.search(r'\b' + table_name + r'\[[^\]]+\]\s*=\s*\{(.*?)\};',
                         data_source, re.S)[1]
        return int(re.findall(r'\{\s*0,\s*(\d+)\s*\}', rows)[kind])
    assert source_count('ftData_Table_Unk0') == 323
    assert source_count('ftData_UnkIntPairs') == 14
    motions = re.findall(r'// ftGw_MS_(\w+) = (\d+)', stock)
    assert len(motions) == 40 and [int(v) for _, v in motions] == list(range(341, 381))
    audio = source('src/melee/lb/lbaudio_ax.static.h')
    palette = re.search(r'lbl_803BB3C0\[[^\]]+\]\s*=\s*\{(.*?)\};', audio, re.S)[1]
    sound_banks = int(re.findall(r'\{\s*0x[0-9A-Fa-f]+,\s*(0x[0-9A-Fa-f]+)\s*\}', palette)[3], 16)
    assert sound_banks == 1 << 29
    receipt = dict(scope='retail asset and source census; not a playable scripted clone',
                   snapshot=str(snapshot), source_sha256=sources,
                   live_article_bindings_sha256=hashlib.sha256(bindings_path.read_bytes()).hexdigest(),
                   retail_sha256={name: hashlib.sha256(value).hexdigest() for name, value in raw.items()},
                   kind=kind, action_count=323, demo_count=14, physical_joints=count,
                   part_to_joint=mapping, joint_to_part=reverse, omission_count=0,
                   costume_joint='PlyGamewatch5K_Share_joint', costume_matanim=None,
                   sound_banks=sound_banks, effect_bank=255, articles=articles,
                   visibility_slot=10, visibility_descriptor=visibility,
                   custom_motions=[dict(name=name, index=int(index)) for name, index in motions])
    output.mkdir(parents=True)
    (output / 'metadata.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(receipt=str(output / 'metadata.json'), physical_joints=count,
                          mapped_parts=38, articles=len(articles), custom_motions=len(motions))))


if __name__ == '__main__':
    main()
