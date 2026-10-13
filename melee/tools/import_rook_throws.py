"""Import the authored Rook grab/throw clips and independent stock victim poses.

Run blender_fighter_animations.py first. This is an explicit authoring operation:
it replaces these clips/events, so do not rerun over subsequent Workshop edits.
All modified files are backed up before writing. The source blend is untouched.
"""
import argparse
from datetime import datetime
import json
from pathlib import Path
import shutil
import struct

from fighter_animation_io import (Dat, animation_bank, disc_file, figatree,
                                  float_track, joints, read_figatree, rebuild_bank)


GRABS = {'Catch': 242, 'CatchDash': 243, 'CatchWait': 244,
         'CatchAttack': 245, 'CatchCut': 246}
THROWS = {'ThrowF': 247, 'ThrowB': 248, 'ThrowHi': 249, 'ThrowLw': 250}
# Authored up-throw first rises above the floor on frame 5.
UP_THROW_TAKEOFF = 5
# Melee's shared skeleton parts, not joint indices in Rook's costume.
PARTS = ('TopN TransN XRotN YRotN HipN WaistN LLegJA LLegJ LKneeJ '
         'LFootJA LFootJ RLegJA RLegJ RKneeJ RFootJA RFootJ BustN '
         'LShoulderN LShoulderJA LShoulderJ LArmJ LHandN '
         'L1stNa L1stNb L2ndNa L2ndNb L3rdNa L3rdNb L4thNa L4thNb '
         'LThumbNa LThumbNb LHandNb NeckN HeadN RShoulderN RShoulderJA '
         'RShoulderJ RArmJ RHandN R1stNa R1stNb R2ndNa R2ndNb '
         'R3rdNa R3rdNb R4thNa R4thNb RThumbNa RThumbNb RHandNb '
         'ThrowN TransN2').split()
WIDTHS = [1,1,1,1,1,2,1,2,1,1] + [
    5,5,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,1,1,
    7,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3,2,1,4]


def commands(dat, at):
    for _ in range(1024):
        opcode = dat.u32(at) >> 26
        if opcode >= len(WIDTHS) or opcode in (5, 7):
            raise ValueError('Expected an inline stock throw script')
        count = WIDTHS[opcode]
        yield at, opcode, bytes(dat.data[at:at + count * 4])
        if opcode == 0:
            return
        at += count * 4
    raise ValueError('Unterminated command stream')


def first_value(track):
    """First authored value, used only to seed a static, editable stock pose."""
    # This deliberately uses the first authored pose, not a time sample of
    # the source clip. Some stock channels have a nonzero pre-roll offset.
    data = track['stream']
    opcode = data[0] & 15
    if opcode not in (1, 2, 3, 4, 6):
        raise ValueError('Pose seed must start with a value-bearing key')
    p = 0
    while data[p] & 128:
        p += 1
    p += 1
    fmt = track['value_format']
    if fmt == 0:
        return struct.unpack_from('<f', data, p)[0]
    types = {32: '<h', 64: '<H', 96: '<b', 128: '<B'}
    return struct.unpack_from(types[fmt & 224], data, p)[0] / (1 << (fmt & 31))


def victim_pose(symbol, source, skeleton, names, source_parts, frames):
    _, original = read_figatree(source)
    if len(original) != len(source_parts):
        raise ValueError('Unexpected source skeleton omissions')
    by_part = {part: node for part, node in zip(source_parts, original)}
    nodes = []
    for joint, name in zip(skeleton, names):
        values = dict(zip((1, 2, 3, 5, 6, 7, 8, 9, 10),
                          (*joint['rotation'], *joint['translation'], *joint['scale'])))
        semantic = 'BustN' if name == 'Bust' else name
        if semantic in PARTS:
            for track in by_part.get(PARTS.index(semantic), []):
                # Stock retargeting retains the destination's limb lengths.
                if track['channel'] in (1, 2, 3):
                    values[track['channel']] = first_value(track)
        if name in ('TopN', 'TransN', 'XRotN', 'YRotN', 'HipN'):
            # Capture constrains XRotN to the thrower's TransN2 (= ThrowN).
            # Remove stock offsets below that anchor so HipN follows it.
            values.update({5: 0.0, 6: 0.0, 7: 0.0})
        nodes.append([float_track(channel, [value] * (int(frames) + 1))
                      for channel, value in values.items()])
    nodes[0] = []  # World position/facing belong to the engine.
    return figatree(symbol, nodes, frames)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--exports', type=Path, required=True)
    parser.add_argument('--backup-root', type=Path, required=True)
    args = parser.parse_args()
    project = args.project.resolve()
    if project == args.backup_root.resolve() or project in args.backup_root.resolve().parents:
        raise ValueError('Backups must live outside the draft')
    manifest = json.loads((args.exports/'manifest.json').read_text())
    fighter = Dat((project/'fighter.dat').read_bytes())
    root = fighter.roots['ftDataRook']
    table = fighter.u32(root + 12)
    clips = animation_bank((project/'animation.dat').read_bytes())
    costume = Dat((project/'costume.dat').read_bytes())
    skeleton = joints(costume, costume.roots['PlyRook5K_Share_joint'])
    names = manifest['bones']
    if len(skeleton) != 50 or len(names) != len(skeleton) or names[48] != 'ThrowN':
        raise ValueError('Unexpected Rook skeleton')
    if fighter.data[fighter.u32(root + 8) + 17] != 48:
        raise ValueError('Rook capture attachment must already use ThrowN')
    purin = Dat(disc_file(args.iso, 'PlPr.dat'))
    pr_root = purin.roots['ftDataPurin']
    pr_table = purin.u32(pr_root + 12)
    pr_clips = animation_bank(disc_file(args.iso, 'PlPrAJ.dat'))
    common = Dat(disc_file(args.iso, 'PlCo.dat'))
    co_root = common.roots['ftLoadCommonData']
    parts = common.u32(common.u32(co_root + 16) + 33 * 4)
    joint_parts = list(common.data[common.u32(parts):common.u32(parts) + common.u32(parts + 8)])
    omissions = common.u32(common.u32(co_root + 20) + 33 * 4)
    omitted = {common.data[common.u32(omissions) + i * 4] for i in range(common.u32(omissions + 4))}
    source_parts = [part for i, part in enumerate(joint_parts) if i not in omitted]
    report = dict(grabs=list(GRABS), throws={}, victim_pose='Stock first pose, editable on Rook skeleton')
    for name, action in (GRABS | THROWS).items():
        at = table + action * 24
        symbol = fighter.string(fighter.u32(at))
        metadata = manifest['actions'][name]
        frames, nodes = read_figatree((args.exports/metadata['file']).read_bytes())
        if len(nodes) != len(skeleton) or frames != metadata['frames']:
            raise ValueError('Export metadata does not match the clip')
        clips[symbol] = figatree(symbol, nodes, frames)
        if name not in THROWS:
            continue
        release = metadata['release']
        if release is None or release != int(release) or not 0 < release <= frames:
            raise ValueError('Release must be an integer frame inside the clip')
        # Use both stock throw-hitbox records (throw and escape damage),
        # but no ordinary attack hits, stock timing, or stock bone references.
        source_commands = list(commands(purin, purin.u32(pr_table + action*24 + 12)))
        damage = b''.join(raw for _, op, raw in source_commands if op == 34)
        if len(damage) != 24:
            raise ValueError('Expected two stock throw-hitbox records')
        if name in ('ThrowF', 'ThrowHi'):
            fighter.word(at + 16, fighter.u32(at + 16) | 0x80000000)
        takeoff = (struct.pack('>2I', (2 << 26) | UP_THROW_TAKEOFF, (25 << 26) | 1)
                   if name == 'ThrowHi' else b'')
        script = damage + takeoff + struct.pack('>3I', (2 << 26) | int(release), 20 << 26, 0)
        target = fighter.u32(at + 12)
        old = list(commands(fighter, target))
        old_end = old[-1][0] + len(old[-1][2])
        other_scripts = [fighter.u32(table + i*24 + 12) for i in range(327) if i != action]
        if (target + len(script) > old_end or target >= table or
                any(target <= p < target+len(script) for p in other_scripts) or
                any(target <= p < target+len(script) for p in fighter.relocs)):
            raise ValueError('Cannot replace this throw script in place')
        fighter.data[target:target+len(script)] = script
        # Four project-owned victim clips. The low flags byte tags the rig;
        # Rook's importer binds source kind 15 to her registered native rig.
        victim = action + 15
        victim_at = table + victim*24
        old_symbol = fighter.string(fighter.u32(victim_at))
        new_symbol = f'PlyRook5K_Share_ACTION_Thrown{name[5:]}_figatree'
        stock_symbol = purin.string(purin.u32(pr_table + victim*24))
        clips.pop(old_symbol)
        clips[new_symbol] = victim_pose(new_symbol, pr_clips[stock_symbol], skeleton,
                                         names, source_parts, frames)
        fighter.pointer(victim_at, fighter.append(new_symbol.encode()+b'\0'))
        fighter.word(victim_at + 16, (fighter.u32(victim_at + 16) & ~255) | 15)
        words = struct.unpack_from('>3I', damage)
        report['throws'][name] = dict(action=action, victim_action=victim,
            release=int(release), end=frames, damage=words[0] & 0x7fffff,
            angle=words[1] >> 23, growth=(words[1] >> 14) & 511,
            weight_knockback=(words[1] >> 5) & 511, base_knockback=words[2] >> 23)
    fighter.data[fighter.u32(root)+0x180] = purin.data[purin.u32(pr_root)+0x180]
    bank = rebuild_bank(fighter, root, clips, 327, 14)
    output = fighter.bytes()
    # Parse rebuilt archives before touching the draft.
    Dat(output)
    animation_bank(bank)
    backup = args.backup_root / ('rook-before-throws-' + datetime.now().strftime('%Y%m%d-%H%M%S'))
    shutil.copytree(project, backup)
    (project/'fighter.dat').write_bytes(output)
    (project/'animation.dat').write_bytes(bank)
    (args.exports/'import-report.json').write_text(json.dumps(report, indent=2))
    print('Backup:', backup)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
