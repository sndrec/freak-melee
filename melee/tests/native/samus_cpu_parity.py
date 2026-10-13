"""Compare Samus's authored CPU decisions with stock in hidden, drawn matches."""
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
    parser.add_argument('--samples', type=int, default=1800,
                        help='Recording duration including full scripted grapple/throw behavior')
    parser.add_argument('--seed', type=lambda value: int(value, 0),
                        help='Override the recording initial RNG seed for both clients')
    args = parser.parse_args()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    tool = str(ROOT / 'build/native/native_content_tool.exe')
    project, store, source, image = (batch / name for name in
                                   ('project', 'store', 'owned.fms', 'portrait.png'))
    portrait(image)
    iso = args.iso.resolve()
    run(tool, 'clone-stock', iso, 'Samus', project, 'Samus CPU comparison')
    run(tool, 'import-parameters', project, ROOT / 'examples/samus_full.fms', source)
    run(tool, 'script-build', project, source)
    package_output = run(tool, '--root', store, 'package-project', project, image)
    package = re.search(r'Cached source fighter ([0-9a-f]{64})', package_output).group(1)
    run(tool, '--root', store, 'verify-package', package)
    run(tool, '--root', store, 'accept', package)
    (batch / 'package.txt').write_text(package)
    base = bytearray((ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes())
    if args.seed is not None:
        struct.pack_into('<I', base, 16, args.seed)
    cases = [
        ('idle-fd-level9', 'final-destination', 9, 'cpu', 'human:zelda', 'idle', None),
        ('idle-fd-level4', 'final-destination', 4, 'cpu', 'human:zelda', 'idle', None),
        ('idle-fd-level5', 'final-destination', 5, 'cpu', 'human:zelda', 'idle', None),
        ('idle-fd-level1', 'final-destination', 1, 'cpu', 'human:zelda', 'idle', None),
        ('shield-fd-level9', 'final-destination', 9, 'cpu', 'human:zelda', 'shield', None),
        ('idle-battlefield-level9', 'battlefield', 9, 'cpu', 'human:zelda', 'idle', None),
        ('recovery-right', 'final-destination', 9, 'cpu', 'human:zelda', 'idle', '1,200,100,-20'),
        ('recovery-left', 'final-destination', 9, 'cpu', 'human:zelda', 'idle', '1,200,-100,-20'),
        ('target-below', 'final-destination', 9, 'cpu', 'human:zelda', 'idle', '1,200,-30,90'),
        ('spin-threat', 'final-destination', 9, 'human', 'cpu:zelda', 'spin', None),
    ]
    results = []
    for name, stage, level, p1, p2, response, airborne in cases:
        if args.case and name not in args.case:
            continue
        frames = bytearray()
        for tick in range(args.samples):
            sample = bytearray(base[24:104])
            if response == 'shield' and tick >= 120:
                struct.pack_into('<Hbb', sample, 24, 1 << 6, 0, 0)
            if response == 'spin':
                buttons, x, y = 0, 0, 0
                if 85 <= tick < 115:
                    x = 80
                if tick >= 200 and tick % 160 == 40:
                    buttons, y = 1 << 9, 100
                struct.pack_into('<Hbb', sample, 8, buttons, x, y)
            frames.extend(sample)
        inputs, directory = batch / (name + '.fmr'), batch / name
        inputs.write_bytes(base[:24] + frames)
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'), '--iso', str(iso),
                   '--inputs', str(inputs), '--stock-fighter', 'samus', '--package-id', package,
                   '--content-root', str(store), '--stocks', '4', '--independent', '--out-dir', str(directory),
                   '--stage', stage, '--p1-type', p1, '--cpu-level', str(level), '--p2', p2]
        if airborne:
            command += ['--fixture-airborne', airborne]
        if name.startswith('recovery') or name == 'spin-threat':
            command += ['--expect-motion', '353' if name == 'spin-threat' else '354']
        if name in ('idle-fd-level9', 'idle-fd-level5'):
            command += ['--expect-motion', '344', '--expect-damage-port', '2']
        print('Comparing Samus CPU: ' + name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        counts = {}
        for label in ('stock', 'clone'):
            log = directory / (name + '-' + label + '.log')
            if log.exists():
                motions = re.findall(r'^\[action\] sample \d+ P1 .*?motion=(\d+) ',
                                     log.read_text(errors='replace'), re.M)
                counts[label] = {motion: motions.count(motion) for motion in sorted(set(motions), key=int)}
        results.append(dict(case=name, package=package, samples=args.samples, seed=args.seed,
                            exit=code, motion_counts=counts))
        (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
