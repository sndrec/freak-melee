"""Replace only Rook clips still byte-identical to stock Jigglypuff.

Run --prepare before Blender export, then --apply with its manifest. The donor
blend is never changed. Existing authored Rook clips and action scripts are
preserved; every proposed replacement must still match the stock disc clip.
"""
import argparse
from datetime import datetime
import json
from pathlib import Path
import shutil

from fighter_animation_io import (Dat, animation_bank, disc_file, figatree,
                                  read_figatree, rebuild_bank)


DAMAGE = (
    'DamageHi1', 'DamageHi2', 'DamageHi3',
    'DamageN1', 'DamageN2', 'DamageN3',
    'DamageLw1', 'DamageLw2', 'DamageLw3',
    'DamageAir1', 'DamageAir2', 'DamageAir3',
    'DamageFlyHi', 'DamageFlyN', 'DamageFlyLw', 'DamageFlyTop', 'DamageFlyRoll',
    'DownDamageU', 'DownDamageD',
)
def inspect(project, iso, names):
    fighter = Dat((project / 'fighter.dat').read_bytes())
    root = fighter.roots['ftDataRook']
    table = fighter.u32(root + 12)
    clips = animation_bank((project / 'animation.dat').read_bytes())
    stock = animation_bank(disc_file(iso, 'PlPrAJ.dat'))
    entries = {}
    for action in range(327):
        at = table + action * 24
        if not fighter.u32(at) or not fighter.u32(at + 8):
            continue
        symbol = fighter.string(fighter.u32(at))
        for name in names:
            if symbol == f'PlyPurin5K_Share_ACTION_{name}_figatree':
                if symbol not in clips or symbol not in stock:
                    raise ValueError(f'Missing Rook or stock clip: {symbol}')
                if clips[symbol] != stock[symbol]:
                    raise ValueError(f'Refusing to overwrite a custom clip: {name}')
                frames, nodes = read_figatree(clips[symbol])
                if len(nodes) != 50:
                    raise ValueError(f'Unexpected skeleton size for {name}')
                entries.setdefault(name, dict(action=action, symbol=symbol,
                                              frames=int(frames)))
    if set(entries) != set(names):
        raise ValueError(f'Unmapped actions: {sorted(set(names) - set(entries))}')
    return fighter, root, clips, entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--exports', type=Path, required=True)
    parser.add_argument('--backup-root', type=Path, required=True)
    parser.add_argument('--prepare', action='store_true')
    args = parser.parse_args()
    names = DAMAGE
    project = args.project.resolve()
    backup_root = args.backup_root.resolve()
    if project == backup_root or project in backup_root.parents:
        raise ValueError('Backups must live outside the draft')
    fighter, root, clips, entries = inspect(project, args.iso, names)
    if args.prepare:
        args.exports.mkdir(parents=True, exist_ok=True)
        (args.exports / 'frame-counts.json').write_text(json.dumps(
            {name: entries[name]['frames'] for name in names}, indent=2) + '\n')
        print('Export actions:', ' '.join(names))
        print('Frame counts:', args.exports / 'frame-counts.json')
        return
    manifest = json.loads((args.exports / 'manifest.json').read_text())
    if set(manifest['actions']) != set(names) or len(manifest['bones']) != 50:
        raise ValueError('Export action set or skeleton does not match the import plan')
    replacements = {}
    for name in names:
        symbol = entries[name]['symbol']
        metadata = manifest['actions'][name]
        raw = (args.exports / metadata['file']).read_bytes()
        frames, nodes = read_figatree(raw)
        if int(frames) != entries[name]['frames'] or len(nodes) != 50:
            raise ValueError(f'Export timing or joint count changed for {name}')
        # Rename the root to the symbol already referenced by Rook's action table.
        replacements[symbol] = figatree(symbol, nodes, frames)
    clips.update(replacements)
    bank = rebuild_bank(fighter, root, clips, 327, 14)
    output = fighter.bytes()
    Dat(output)
    rebuilt = animation_bank(bank)
    if any(rebuilt[name] != clip for name, clip in clips.items()):
        raise ValueError('Rebuilt bank did not retain every clip')
    backup = backup_root / ('rook-before-damage-animations-' +
                            datetime.now().strftime('%Y%m%d-%H%M%S'))
    shutil.copytree(project, backup)
    (project / 'fighter.dat').write_bytes(output)
    (project / 'animation.dat').write_bytes(bank)
    report = {name: dict(action=entries[name]['action'], frames=entries[name]['frames'],
                         source_frames=manifest['actions'][name]['source_frames']) for name in names}
    (args.exports / 'import-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Backup:', backup)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
