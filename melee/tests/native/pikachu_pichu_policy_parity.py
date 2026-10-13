"""Compare Pikachu/Pichu common attacks, held items and CPU policies with stock."""
import argparse
import json
from pathlib import Path
import re
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
    cases = [('jab-repeat', 'final-destination', 9, 'jab'),
             ('forward-smash-contact', 'final-destination', 9, 'smash'),
             ('held-bat-up-special', 'final-destination', 9, 'bat'),
             ('cpu-idle-level9', 'final-destination', 9, 'idle'),
             ('cpu-shield-level9', 'final-destination', 9, 'shield'),
             ('cpu-idle-level4', 'final-destination', 4, 'idle'),
             ('cpu-battlefield-level9', 'battlefield', 9, 'idle')]
    results = []
    for fighter in ('Pikachu', 'Pichu'):
        project, store = batch / fighter, batch / (fighter + '-store')
        source, image = batch / (fighter + '.fms'), batch / (fighter + '.png')
        portrait(image)
        run(tool, 'clone-stock', iso, fighter, project, fighter + ' policy comparison')
        run(tool, 'import-parameters', project, ROOT / 'examples' / (fighter.lower() + '_wip.fms'), source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        for suffix, stage, level, response in cases:
            name = fighter.lower() + '-' + suffix
            if args.case and name not in args.case:
                continue
            cpu = suffix.startswith('cpu-')
            frames = bytearray()
            for tick in range(2200 if cpu else 700):
                sample = bytearray(base[24:104])
                buttons, x, y = 0, 0, 0
                if response == 'jab' and 200 <= tick < 400 and tick % 6 == 2:
                    buttons = 1 << 8
                if response == 'smash' and tick == 300:
                    buttons, x = 1 << 8, 80
                if response == 'bat':
                    if tick == 220: buttons = 1 << 8
                    if tick == 280: buttons, y = 1 << 9, 80
                    if tick == 500: buttons = 16
                struct.pack_into('<Hbb', sample, 8, buttons, x, y)
                if response == 'smash' and 100 <= tick < 196:
                    struct.pack_into('<Hbb', sample, 24, 0, -80, 0)
                if response == 'shield' and tick >= 120:
                    struct.pack_into('<Hbb', sample, 24, 1 << 6, 0, 0)
                frames.extend(sample)
            inputs, directory = batch / (name + '.fmr'), batch / name
            inputs.write_bytes(base[:24] + frames)
            command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                       '--iso', str(iso), '--inputs', str(inputs),
                       '--stock-fighter', fighter.lower(), '--package-id', package,
                       '--content-root', str(store), '--stocks', '4' if cpu else '2',
                       '--p2', 'human:zelda', '--out-dir', str(directory),
                       '--stage', stage, '--cpu-level', str(level)]
            if cpu:
                command += ['--p1-type', 'cpu', '--expect-damage-port', '2']
            elif response == 'jab':
                command += ['--expect-motion', '44']
            elif response == 'smash':
                command += ['--expect-motion', '60', '--expect-damage-port', '2']
            else:
                command += ['--fixture-item', '11,120,-55,10', '--expect-item-kind', '11',
                            '--expect-motion', '92', '--expect-motion', '353']
            print('Comparing ' + name, flush=True)
            code = subprocess.run(command, cwd=ROOT).returncode
            counts = {}
            for label in ('stock', 'clone'):
                log = directory / (name + '-' + label + '.log')
                if log.exists():
                    text = log.read_text(errors='replace')
                    motions = re.findall(r'^\[action\] sample \d+ P1 .*?motion=(\d+) ', text, re.M)
                    counts[label] = {motion: motions.count(motion)
                                     for motion in sorted(set(motions), key=int)}
                    if response == 'jab' and code == 0:
                        # The action logger omits repeated changes to the same
                        # motion ID. Count real animation restarts instead.
                        poses = re.findall(r'^\[fighter\] sample \d+ P1 motion=44 frame=([0-9a-f]+) ', text, re.M)
                        frame_values = [struct.unpack('<f', bytes.fromhex(bits)[::-1])[0] for bits in poses]
                        restarts = sum(current < previous for previous, current
                                       in zip(frame_values, frame_values[1:]))
                        counts[label]['jab_animation_restarts'] = restarts
                        if restarts < 2 or '45' in motions:
                            print('Repeating first jab was not exercised in ' + label)
                            code = 1
            results.append(dict(case=name, package=package, exit=code, motion_counts=counts))
            (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
