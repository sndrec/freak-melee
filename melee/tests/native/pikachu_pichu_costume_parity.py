"""Compare retail costume pose/visibility with independently packaged authored clones."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from fighter_script_bundle_roundtrip import run, portrait, bundled_id

ROOT = Path(__file__).resolve().parents[2]


def independent_costume(data, fighter, tag):
    for suffix in ('joint', 'matanim_joint'):
        old = ('Ply' + fighter + '5K' + tag + '_Share_' + suffix).encode() + b'\0'
        new = ('Ply' + fighter + '5K_Share_' + suffix).encode() + b'\0'
        assert data.count(old) == 1
        data = data.replace(old, new + b'\0' * (len(old) - len(new)))
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--out-dir', required=True, type=Path)
    parser.add_argument('--case', action='append')
    args = parser.parse_args()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    tool = str(ROOT / 'build/native/native_content_tool.exe')
    iso = args.iso.resolve()
    base = (ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    results = []
    for fighter in ('Pikachu', 'Pichu'):
        project, store = batch / fighter, batch / (fighter + '-store')
        source, image = batch / (fighter + '.fms'), batch / (fighter + '.png')
        portrait(image)
        run(tool, 'clone-stock', iso, fighter, project, fighter + ' costume comparison')
        run(tool, 'import-parameters', project, ROOT / 'examples' / (fighter.lower() + '_wip.fms'), source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        skins = {}
        for color, tag in enumerate(('Re', 'Bu', 'Gr'), 1):
            original = batch / (fighter + '-retail-' + str(color) + '.dat')
            run(tool, 'extract-stock', iso, fighter, color, original)
            path = batch / (fighter + '-costume-' + str(color) + '.dat')
            path.write_bytes(independent_costume(original.read_bytes(), fighter, tag))
            output = run(tool, '--root', store, 'import-custom-costume', path, package,
                         fighter + ' ' + tag, color)
            skins[color] = re.search(r'ID: ([a-f0-9]{64})', output).group(1)
        (batch / (fighter + '-skins.json')).write_text(json.dumps(skins, indent=2) + '\n')
        for color in range(4):
            for move in ('thunder', 'up-special'):
                name = fighter.lower() + '-color-' + str(color) + '-' + move
                if args.case and name not in args.case:
                    continue
                frames = bytearray()
                for tick in range(700):
                    sample = bytearray(base[24:104])
                    buttons, x, y = 0, 0, 0
                    if tick == 150: buttons, y = 1 << 9, -80 if move == 'thunder' else 80
                    elif move == 'up-special' and 150 < tick < 215:
                        x, y = (0, 80) if tick >= 167 else (80, 0)
                    struct.pack_into('<Hbb', sample, 8, buttons, x, y)
                    frames.extend(sample)
                inputs, directory = batch / (name + '.fmr'), batch / name
                inputs.write_bytes(base[:24] + frames)
                command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                           '--iso', str(iso), '--inputs', str(inputs),
                           '--stock-fighter', fighter.lower(), '--stock-color', str(color),
                           '--package-id', package, '--content-root', str(store), '--stocks', '2',
                           '--out-dir', str(directory)]
                if color: command += ['--clone-skin', skins[color]]
                if move == 'thunder':
                    command += ['--expect-motion', '359', '--expect-motion', '361',
                                '--expect-item-kind', '81' if fighter == 'Pikachu' else '82']
                else:
                    command += ['--expect-motion', '353', '--expect-motion', '354', '--expect-motion', '357']
                print('Comparing ' + name, flush=True)
                code = subprocess.run(command, cwd=ROOT).returncode
                results.append(dict(case=name, fighter=fighter, color=color, move=move,
                                    package=package, skin=skins.get(color), exit=code))
                (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
