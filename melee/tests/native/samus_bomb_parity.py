"""Compare authored Samus bombs and morph movement against stock callbacks."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from fighter_script_bundle_roundtrip import run, portrait

ROOT = Path(__file__).resolve().parents[2]


def coverage(directory, name):
    for label in ('stock', 'clone'):
        lines = (directory / (name + '-' + label + '.log')).read_text(
            encoding='utf-8', errors='replace').splitlines()
        items = [m.groups() for line in lines if (m := re.match(
            r'\[item\] sample (\d+) I\d+ kind=93 state=(\d+) ', line))]
        if not items or not any(row[1] == '3' for row in items) or max(int(row[0]) for row in items) >= 899:
            print(f'{label} missed bomb creation, explosion or removal: {name}', flush=True)
            return False
        if '-ground-' in name or '-contact-' in name:
            if not any(re.match(r'\[action\] sample \d+ P1 .*motion=342 ', line) for line in lines):
                print(f'{label} missed the bomb-induced jump: {name}', flush=True)
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
    source, image = batch / 'bombs.fms', batch / 'portrait.png'
    portrait(image)
    run(tool, 'clone-stock', iso, 'Samus', project, 'Samus bomb comparison')
    run(tool, 'import-parameters', project, ROOT / 'examples/samus_full.fms', source)
    run(tool, 'script-build', project, source)
    package_output = run(tool, '--root', store, 'package-project', project, image)
    package = re.search(r'Cached source fighter ([0-9a-f]{64})', package_output).group(1)
    run(tool, '--root', store, 'verify-package', package)
    run(tool, '--root', store, 'accept', package)
    base = (ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    cases = [(place, direction) for place in ('ground', 'air', 'crouch', 'drift', 'repeat', 'contact')
             for direction in ('right', 'left')]
    results = []
    for place, direction in cases:
        name = f'samus-bomb-{place}-{direction}'
        if args.case and name not in args.case:
            continue
        sign = -1 if direction == 'left' else 1
        shots = (200, 270, 340, 410) if place == 'repeat' else (200,)
        frames = bytearray()
        for tick in range(900):
            sample = bytearray(base[24:104])
            buttons, x, y = 0, 0, 0
            if direction == 'left' and 80 <= tick < 100: x = -80
            if place == 'air' and tick == 180: buttons = 1024
            if place == 'crouch' and 170 <= tick < 230: y = -80
            if place == 'drift' and 201 <= tick < 240: x = sign * 60
            if tick in shots: buttons, y = 512, -80
            struct.pack_into('<Hbb', sample, 8, buttons, x, y)
            struct.pack_into('<Hbb', sample, 24, 0, 0, 0)
            frames.extend(sample)
        inputs, directory = batch / (name + '.fmr'), batch / name
        inputs.write_bytes(base[:24] + frames)
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                   '--iso', str(iso), '--inputs', str(inputs), '--stock-fighter', 'samus',
                   '--package-id', package, '--content-root', str(store), '--stocks', '2', '--independent',
                   '--out-dir', str(directory), '--expect-motion', '356' if place == 'air' else '355',
                   '--expect-item-kind', '93']
        if place == 'contact':
            bomb_x = -60 if direction == 'right' else -65.89
            command += ['--fixture-airborne', f'2,265,{bomb_x},5', '--expect-damage-port', '2']
        print('Comparing ' + name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        if code == 0 and not coverage(directory, name): code = 1
        results.append(dict(case=name, package=package, exit=code))
        (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
