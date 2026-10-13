"""Compare authored Samus charge storage, release and projectile contact."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from fighter_script_bundle_roundtrip import run, portrait

ROOT = Path(__file__).resolve().parents[2]


def check_flight_coverage(directory, name, mode):
    if mode not in ('full-left', 'stored-spin'):
        return True
    state, after = (8, 700) if mode == 'full-left' else (6, 750)
    for label in ('stock', 'clone'):
        lines = (directory / (name + '-' + label + '.log')).read_text(
            encoding='utf-8', errors='replace').splitlines()
        if not any((match := re.match(r'\[item\] sample (\d+) I\d+ kind=94 state=(\d+) ', line))
                   and int(match[1]) >= after and int(match[2]) == state for line in lines):
            print(f'{label} did not trace the required later state-{state} projectile', flush=True)
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
    source, image = batch / 'charge.fms', batch / 'portrait.png'
    portrait(image)
    run(tool, 'clone-stock', iso, 'Samus', project, 'Samus charge comparison')
    run(tool, 'import-parameters', project, ROOT / 'examples/samus_full.fms', source)
    run(tool, 'script-build', project, source)
    package_output = run(tool, '--root', store, 'package-project', project, image)
    package = re.search(r'Cached source fighter ([0-9a-f]{64})', package_output).group(1)
    run(tool, '--root', store, 'verify-package', package)
    run(tool, '--root', store, 'accept', package)
    base = (ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    results = []
    for mode in ('partial-right', 'partial-left', 'full-right', 'full-left', 'cancel-resume',
                 'air-right', 'air-left', 'charging-damage', 'stored-spin'):
        name = 'samus-charge-' + mode
        if args.case and name not in args.case:
            continue
        frames = bytearray()
        for tick in range(1100):
            sample = bytearray(base[24:104])
            buttons, x, y = 0, 0, 0
            if mode.endswith('left') and 80 <= tick < 100: x = -80
            if mode.startswith('air') and tick == 180: buttons = 1024
            if tick == 200: buttons = 512
            if mode.startswith('partial') and tick == 270: buttons = 512
            if mode.startswith('full') and tick == 700: buttons = 512
            if mode in ('cancel-resume', 'stored-spin') and tick == 270: buttons = 64
            if mode == 'cancel-resume' and tick in (400, 450): buttons = 512
            if mode == 'stored-spin' and tick == 400: buttons, y = 512, 80
            if mode == 'stored-spin' and tick in (700, 750): buttons = 512
            struct.pack_into('<Hbb', sample, 8, buttons, x, y)
            other_buttons = other_x = 0
            if not mode.endswith('left') and 100 <= tick < 196: other_x = -80
            if mode == 'charging-damage' and tick == 260: other_buttons = 256
            struct.pack_into('<Hbb', sample, 24, other_buttons, other_x, 0)
            frames.extend(sample)
        inputs, directory = batch / (name + '.fmr'), batch / name
        inputs.write_bytes(base[:24] + frames)
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                   '--iso', str(iso), '--inputs', str(inputs), '--stock-fighter', 'samus',
                   '--package-id', package, '--content-root', str(store), '--stocks', '2', '--independent',
                   '--out-dir', str(directory), '--expect-motion', '347' if mode.startswith('air') else '343',
                   '--expect-item-kind', '94']
        if mode != 'charging-damage':
            command += ['--expect-motion', '348' if mode.startswith('air') else '346']
        if mode in ('cancel-resume', 'full-right', 'full-left', 'stored-spin'):
            command += ['--expect-motion', '345']
        if mode == 'stored-spin': command += ['--expect-motion', '353']
        if mode in ('partial-right', 'full-right', 'cancel-resume'):
            command += ['--expect-damage-port', '2']
        if mode == 'charging-damage':
            command += ['--expect-damage-port', '1', '--expect-item-removed-on-damage', '94', '1']
        print('Comparing ' + name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        if code == 0 and not check_flight_coverage(directory, name, mode): code = 1
        results.append(dict(case=name, package=package, exit=code))
        (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
