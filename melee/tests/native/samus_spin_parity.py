"""Compare authored Samus Screw Attack with stock entry, turn, flight and contact."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
from fighter_script_bundle_roundtrip import run, portrait

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
    project, store = batch / 'Samus', batch / 'store'
    source, image = batch / 'spin.fms', batch / 'portrait.png'
    portrait(image)
    run(tool, 'clone-stock', iso, 'Samus', project, 'Samus Screw Attack comparison')
    run(tool, 'import-parameters', project, ROOT / 'examples/samus_full.fms', source)
    run(tool, 'script-build', project, source)
    package_output = run(tool, '--root', store, 'package-project', project, image)
    package = re.search(r'Cached source fighter ([0-9a-f]{64})', package_output).group(1)
    run(tool, '--root', store, 'verify-package', package)
    run(tool, '--root', store, 'accept', package)
    base = (ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    results = []
    for air in (False, True):
        for mode in ('right', 'left', 'turn', 'contact'):
            name = 'samus-spin-' + ('air-' if air else 'ground-') + mode
            if args.case and name not in args.case:
                continue
            frames = bytearray()
            for tick in range(700):
                sample = bytearray(base[24:104])
                buttons, x, y = 0, 0, 0
                if mode == 'left' and 80 <= tick < 100: x = -80
                if air and tick == 180: buttons = 1 << 10
                if tick == 200: buttons, y = 1 << 9, 80
                if mode == 'turn' and 201 <= tick < 220: x = -80
                struct.pack_into('<Hbb', sample, 8, buttons, x, y)
                if mode == 'contact' and 100 <= tick < 196:
                    struct.pack_into('<Hbb', sample, 24, 0, -80, 0)
                frames.extend(sample)
            inputs, directory = batch / (name + '.fmr'), batch / name
            inputs.write_bytes(base[:24] + frames)
            command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                       '--iso', str(iso), '--inputs', str(inputs), '--stock-fighter', 'samus',
                       '--package-id', package, '--content-root', str(store), '--stocks', '2', '--independent',
                       '--out-dir', str(directory), '--expect-motion', '354' if air else '353',
                       '--expect-motion', '35']
            # Grounded turnaround travels off the edge and catches the ledge.
            # The other recordings land on-stage after entering freefall.
            if not air and mode == 'turn':
                command += ['--expect-motion', '12', '--expect-motion', '13']
            else:
                command += ['--expect-motion', '43']
            if mode == 'contact': command += ['--expect-damage-port', '2']
            if mode == 'contact' and air:
                command += ['--fixture-airborne', '2,201,-55,25']
            print('Comparing ' + name, flush=True)
            code = subprocess.run(command, cwd=ROOT).returncode
            if code == 0 and mode in ('left', 'turn'):
                for label in ('stock', 'clone'):
                    text = (directory / (name + '-' + label + '.log')).read_text(errors='replace')
                    if not re.search(r'^\[fighter\] sample \d+ P1 motion=35[34] .*?facing=bf800000 ', text, re.M):
                        print('Left-facing Screw Attack was not exercised: ' + name, flush=True)
                        code = 1
            if code == 0 and mode == 'contact':
                for label in ('stock', 'clone'):
                    text = (directory / (name + '-' + label + '.log')).read_text(errors='replace')
                    if not re.search(r'^\[fighter\] sample \d+ P1 motion=35[34] .*?hitlag=(?!00000000)[0-9a-f]{8}', text, re.M):
                        print('Screw Attack contact hitlag was not exercised: ' + name, flush=True)
                        code = 1
            results.append(dict(case=name, package=package, exit=code))
            (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
