"""Compare Bowser's authored up/down specials in hidden, normally drawn matches.

Selected-move acceptance only: remaining fighter-wide policies are stock in the
partial template. Every run retains both traces.
"""
import argparse
import datetime
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from fighter_script_bundle_roundtrip import run, portrait, bundled_id

ROOT = Path(__file__).resolve().parents[2]


def recording(path, move, mode, direction, steer, contact):
    baseline = (ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    frames = bytearray()
    start = 170 if mode == 'air' else 150
    # A longer approach reaches the target, but Bowser must finish RunBrake
    # before the special input. A B tap during that noninterruptible segment
    # only leaves a buffered directional input and never enters the special.
    if contact: start = 230
    for tick in range(550):
        sample = bytearray(baseline[24:104])
        buttons, x, y = 0, 0, 0
        if direction == 'left' and 100 <= tick < 102: x = -40
        if contact and 85 <= tick < 151: x = 100
        if mode == 'air' and 145 <= tick < 153: buttons = 1 << 10
        if tick == start: buttons, y = 1 << 9, 80 if move == 'up' else -80
        if steer and start < tick < start + 55:
            x = 80 if direction == 'left' else -80
        struct.pack_into('<Hbb', sample, 8, buttons, x, y)
        frames.extend(sample)
    path.write_bytes(baseline[:24] + frames)


def coverage(directory, name, move, mode, direction):
    entry = (359 if mode == 'ground' else 360) if move == 'up' else (361 if mode == 'ground' else 362)
    facing = 'bf800000' if direction == 'left' else '3f800000'
    for label in ('stock', 'clone'):
        states = re.findall(r'^\[fighter\] sample \d+ P1 (.*)$',
                            (directory / f'{name}-{label}.log').read_text(errors='replace'), re.M)
        if not any(state.startswith(f'motion={entry} ') and 'facing=' + facing in state for state in states):
            print(f'Missing expected entry/facing in {name} {label}', flush=True)
            return False
        if move == 'down':
            # Starting at the left spawn and facing outwards, the grounded
            # animation's root motion carries stock Bowser past the ledge.
            # Require the actual KO/respawn path instead of claiming a landing.
            required_states = (362, 0, 12, 13) if mode == 'ground' and direction == 'left' else (362, 363, 14)
            for required in required_states:
                if not any(state.startswith(f'motion={required} ') for state in states):
                    print(f'Missing dive/landing/finish {required} in {name} {label}', flush=True)
                    return False
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--out-dir', type=Path, default=ROOT / '.local' /
                        ('bowser-special-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S')))
    parser.add_argument('--case', action='append')
    args = parser.parse_args()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    iso = args.iso.resolve()
    project, store = batch / 'project', batch / 'store'
    image, source = batch / 'portrait.png', batch / 'owned.fms'
    portrait(image)
    tool = str(ROOT / 'build/native/native_content_tool.exe')
    run(tool, 'clone-stock', iso, 'Bowser', project, 'Bowser special comparison')
    run(tool, 'import-parameters', project, ROOT / 'examples/koopa_wip.fms', source)
    run(tool, 'script-build', project, source)
    package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
    run(tool, '--root', store, 'accept', package, iso)
    (batch / 'package.txt').write_text(package)
    cases = []
    for move in ('up', 'down'):
        for mode in ('ground', 'air'):
            for direction in ('right', 'left'):
                for steer in ((False, True) if move == 'up' else (False,)):
                    name = f'{move}-{mode}-{direction}' + ('-steer' if steer else '')
                    cases.append((name, move, mode, direction, steer, False, 'final-destination'))
        cases.append((move + '-ground-contact', move, 'ground', 'right', False, True, 'final-destination'))
        cases.append((move + '-ground-slope', move, 'ground', 'right', False, False, 'yoshis-story'))
    results = []
    for name, move, mode, direction, steer, contact, stage in cases:
        if args.case and name not in args.case: continue
        print('Comparing Bowser: ' + name, flush=True)
        inputs, directory = batch / (name + '.fmr'), batch / name
        recording(inputs, move, mode, direction, steer, contact)
        motion = (359 if mode == 'ground' else 360) if move == 'up' else (361 if mode == 'ground' else 362)
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                   '--iso', str(iso), '--inputs', str(inputs), '--stock-fighter', 'bowser',
                   '--package-id', package, '--content-root', str(store), '--stocks', '2',
                   '--out-dir', str(directory), '--expect-motion', str(motion), '--stage', stage]
        if contact: command.extend(('--expect-damage-port', '2'))
        result = subprocess.run(command, cwd=ROOT)
        code = result.returncode
        if not code and not coverage(directory, name, move, mode, direction): code = 1
        results.append(dict(case=name, package=package, exit=code))
        (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
