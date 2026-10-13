"""Compare scripted Samus missiles with the original fighter and item callbacks."""
import argparse
import json
import re
from pathlib import Path
import struct
import subprocess
import sys
from fighter_script_bundle_roundtrip import run, portrait

ROOT = Path(__file__).resolve().parents[2]


def check_coverage(directory, name, variant, place, direction):
    for label in ('stock', 'clone'):
        lines = (directory / (name + '-' + label + '.log')).read_text(
            encoding='utf-8', errors='replace').splitlines()
        items = [m.groups() for line in lines if (m := re.match(
            r'\[item\] sample (\d+) I\d+ kind=95 state=(\d+).*vel=\(([-\d.]+),([-\d.]+)\)', line))]
        flight, explosion = (0, 2) if variant == 'homing' else (1, 3)
        flights = [row for row in items if int(row[1]) == flight]
        sign = -1 if direction == 'left' else 1
        valid = (flights and any(float(row[2]) * sign > 0 for row in flights)
                 and any(int(row[1]) == explosion for row in items)
                 and max(int(row[0]) for row in items) < 899)
        if place == 'target': valid = valid and any(float(row[3]) != 0 for row in flights)
        if place == 'repeat':
            motion = 349 + (variant == 'super')
            valid = valid and sum(bool(re.match(rf'\[action\] sample \d+ P1 .*motion={motion} ', line))
                                  for line in lines) >= 3
        if not valid:
            print(f'{label} missed flight, facing, explosion/removal or case coverage for {name}', flush=True)
            return False
    return True


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
    project, store = batch / 'Samus', batch / 'store'
    source, image = batch / 'missiles.fms', batch / 'portrait.png'
    portrait(image)
    run(tool, 'clone-stock', iso, 'Samus', project, 'Samus missile comparison')
    run(tool, 'import-parameters', project, ROOT / 'examples/samus_full.fms', source)
    run(tool, 'script-build', project, source)
    package_output = run(tool, '--root', store, 'package-project', project, image)
    package = re.search(r'Cached source fighter ([0-9a-f]{64})', package_output).group(1)
    run(tool, '--root', store, 'verify-package', package)
    run(tool, '--root', store, 'accept', package)
    base = (ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    cases = [(variant, place, direction) for variant in ('homing', 'super')
             for place in ('ground', 'air') for direction in ('right', 'left')]
    cases += [('homing', 'target', 'right'), ('homing', 'target', 'left'),
              ('homing', 'repeat', 'right'), ('super', 'repeat', 'right')]
    results = []
    for variant, place, direction in cases:
        name = f'samus-missile-{variant}-{place}-{direction}'
        if args.case and name not in args.case:
            continue
        sign = -1 if direction == 'left' else 1
        shots = (200, 280, 360) if place == 'repeat' else (200,)
        frames = bytearray()
        for tick in range(900):
            sample = bytearray(base[24:104])
            buttons, x = 0, 0
            if direction == 'left' and 80 <= tick < 100: x = -80
            if place == 'air' and tick == 180: buttons = 1024
            if variant == 'homing' and any(shot - 10 <= tick <= shot for shot in shots): x = 80 * sign
            if tick in shots: buttons, x = 512, 80 * sign
            struct.pack_into('<Hbb', sample, 8, buttons, x, 0)
            struct.pack_into('<Hbb', sample, 24, 0, 0, 0)
            frames.extend(sample)
        inputs, directory = batch / (name + '.fmr'), batch / name
        inputs.write_bytes(base[:24] + frames)
        motion = (351 if place == 'air' else 349) + (variant == 'super')
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                   '--iso', str(iso), '--inputs', str(inputs), '--stock-fighter', 'samus',
                   '--package-id', package, '--content-root', str(store), '--stocks', '2', '--independent',
                   '--out-dir', str(directory), '--expect-motion', str(motion),
                   '--expect-item-kind', '95']
        if place == 'ground' and direction == 'right': command += ['--expect-damage-port', '2']
        if place == 'target':
            command += ['--fixture-airborne', f'2,210,{0 if sign > 0 else -140},45']
        if place == 'repeat': command += ['--fixture-airborne', '2,199,-200,200']
        print('Comparing ' + name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        if code == 0 and not check_coverage(directory, name, variant, place, direction): code = 1
        results.append(dict(case=name, package=package, exit=code))
        (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
