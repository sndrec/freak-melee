"""Compare package-owned DK CPU behavior with stock, with hidden drawn clients."""

import argparse
import datetime
import json
from pathlib import Path
import re
import subprocess
import struct
import sys
from fighter_script_bundle_roundtrip import run, portrait, bundled_id

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, default=ROOT / '.local' /
                        ('donkey-cpu-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S')))
    parser.add_argument('--case-prefix')
    args = parser.parse_args()
    args.out_dir.mkdir(parents=True, exist_ok=False)
    tool = str(ROOT / 'build/native/native_content_tool.exe')
    project, store, source = args.out_dir / 'project', args.out_dir / 'store', args.out_dir / 'owned.fms'
    image = args.out_dir / 'portrait.png'
    portrait(image)
    run(tool, 'clone-stock', args.iso, 'Donkey Kong', project, 'Donkey CPU Parity')
    run(tool, 'import-parameters', project, ROOT / 'examples/donkey_wip.fms', source)
    run(tool, 'script-build', project, source)
    package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, args.iso))
    run(tool, '--root', store, 'accept', package, args.iso)
    baseline = (ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    cases = [(stage, level, 'human:zelda', 'idle') for stage, level in
             [('final-destination', 9), ('battlefield', 9), ('final-destination', 4), ('final-destination', 1)]]
    cases.extend((stage, 9, 'human:zelda', 'shield') for stage in ('final-destination', 'battlefield'))
    results = []
    for stage, level, opponent, response in cases:
        case = response + f'-{stage}-level{level}-' + opponent.replace(':', '-')
        if args.case_prefix and not case.startswith(args.case_prefix): continue
        inputs = args.out_dir / (case + '.fmr')
        samples = bytearray()
        for tick in range(3000):
            sample = bytearray(baseline[24:104])
            if response == 'shield' and tick >= 120:
                struct.pack_into('<Hbb', sample, 24, 1 << 6, 0, 0)
            samples.extend(sample)
        inputs.write_bytes(baseline[:24] + samples)
        directory = args.out_dir / case
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'), '--iso', str(args.iso.resolve()),
                   '--inputs', str(inputs.resolve()), '--stock-fighter', 'donkeykong', '--package-id', package,
                   '--content-root', str(store.resolve()), '--stocks', '4', '--out-dir', str(directory),
                   '--stage', stage, '--p1-type', 'cpu', '--cpu-level', str(level), '--p2', opponent,
                   '--expect-damage-port', '2']
        print('Comparing ' + case, flush=True)
        result = subprocess.run(command, cwd=ROOT)
        motion_counts = {}
        for label in ('stock', 'clone'):
            log = directory / f'{case}-{label}.log'
            if log.exists():
                motions = re.findall(r'^\[action\] sample \d+ P1 .*?motion=(\d+) ', log.read_text(errors='replace'), re.M)
                motion_counts[label] = {motion: motions.count(motion) for motion in sorted(set(motions), key=int)}
        results.append(dict(case=case, package=package, exit=result.returncode, motion_counts=motion_counts))
    (args.out_dir / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return 0 if results and all(row['exit'] == 0 for row in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
