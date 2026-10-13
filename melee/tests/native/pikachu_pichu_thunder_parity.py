"""Compare authored Thunder fighter phases and chained articles with stock."""
import argparse
import json
import re
from pathlib import Path
import struct
import subprocess
import sys
from fighter_script_bundle_roundtrip import run, portrait, bundled_id

ROOT = Path(__file__).resolve().parents[2]


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
    cases = [('ground-right', False, 1, 'final-destination'),
             ('ground-left', False, -1, 'final-destination'),
             ('air-right', True, 1, 'final-destination'),
             ('air-left', True, -1, 'final-destination'),
             ('air-catch-right', True, 1, 'final-destination'),
             ('air-catch-left', True, -1, 'final-destination'),
             ('repeat', False, 1, 'final-destination'),
             ('slope', False, 1, 'yoshis-story'),
             ('platform', True, 1, 'battlefield'),
             ('offstage', True, -1, 'final-destination'),
             ('victim', False, 1, 'final-destination'),
             ('shield', False, 1, 'final-destination'),
             ('interrupted', False, 1, 'final-destination'),
             ('absorb', False, 1, 'final-destination')]
    results = []
    for fighter, kind in [('Pikachu', 81), ('Pichu', 82)]:
        project, store = batch / fighter, batch / (fighter + '-store')
        source, image = batch / (fighter + '.fms'), batch / (fighter + '.png')
        portrait(image)
        run(tool, 'clone-stock', iso, fighter, project, fighter + ' Thunder comparison')
        run(tool, 'import-parameters', project, ROOT / 'examples' / (fighter.lower() + '_wip.fms'), source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        for suffix, air, direction, stage in cases:
            name = fighter.lower() + '-' + suffix
            if args.case and name not in args.case:
                continue
            start = 180 if air else 150
            interaction = suffix in ('victim', 'shield', 'interrupted', 'absorb')
            if interaction:
                start = 300
            frames = bytearray()
            for tick in range(700):
                sample = bytearray(base[24:104])
                buttons, x, y = 0, 0, 0
                if direction < 0 and 100 <= tick < 102:
                    x = -40
                if suffix == 'offstage' and 100 <= tick < 135:
                    x = -80
                if air and 145 <= tick < 153:
                    buttons = 1 << 10
                if suffix.startswith('air-catch') and 168 <= tick < 171:
                    buttons = 1 << 10
                if tick == start or suffix == 'repeat' and tick in (300, 450):
                    buttons, y = 1 << 9, -80
                struct.pack_into('<Hbb', sample, 8, buttons, x, y)
                if interaction:
                    p2_buttons, p2_x, p2_y = 0, 0, 0
                    if 100 <= tick < (174 if suffix == 'absorb' else 164):
                        p2_x = -80
                    if suffix == 'shield' and 270 <= tick < 400:
                        p2_buttons = 1 << 6
                    if suffix == 'interrupted' and tick == 323:
                        p2_buttons, p2_y = 1 << 8, 80
                    if suffix == 'absorb' and 310 <= tick < 318:
                        p2_buttons = 1 << 10
                    # Catch a segment above the summoner. A grounded Magnet
                    # sits below the segment before Pikachu catches its tip.
                    if suffix == 'absorb' and 324 <= tick < 400:
                        p2_buttons, p2_y = 1 << 9, -80
                    struct.pack_into('<Hbb', sample, 24, p2_buttons, p2_x, p2_y)
                frames.extend(sample)
            inputs = batch / (name + '.fmr')
            inputs.write_bytes(base[:24] + frames)
            command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                       '--iso', str(iso), '--inputs', str(inputs),
                       '--stock-fighter', fighter.lower(), '--package-id', package,
                       '--content-root', str(store), '--stocks', '2',
                       '--p2', 'human:ness' if suffix == 'absorb' else 'human:mario' if interaction else 'human:zelda',
                       '--out-dir', str(batch / name), '--stage', stage,
                       '--expect-motion', '363' if air else '359',
                       '--expect-item-kind', str(kind)]
            if suffix not in ('offstage', 'interrupted', 'absorb', 'platform'):
                command += ['--expect-motion', '365' if suffix.startswith('air-catch') else '361']
            if suffix == 'platform':
                command += ['--expect-motion', '362']
            if suffix == 'victim':
                command += ['--expect-damage-port', '2']
            elif suffix == 'interrupted':
                command += ['--expect-damage-port', '1']
            elif suffix == 'shield':
                command += ['--expect-port-motion', '2', '181']
            elif suffix == 'absorb':
                command += ['--expect-item-event', 'absorb', str(kind)]
            print('Comparing ' + name, flush=True)
            code = subprocess.run(command, cwd=ROOT).returncode
            if suffix == 'interrupted' and code == 0:
                # Pichu's copied subactions deal self-damage; that alone does
                # not prove the opponent interrupted Thunder.
                for label in ('stock', 'clone'):
                    log = (batch / name / (name + '-' + label + '.log')).read_text(errors='replace')
                    contacts = re.findall(r'^\[fighter\] sample (\d+) P1 motion=(\d+) ', log, re.M)
                    if not any(300 < int(sample) < 400 and 75 <= int(motion) <= 91
                               for sample, motion in contacts):
                        print('No opponent-induced damage motion in ' + label)
                        code = 1
            results.append(dict(case=name, package=package, exit=code))
            (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
