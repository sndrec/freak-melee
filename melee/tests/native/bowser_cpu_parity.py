"""Compare Bowser's owned CPU policies in hidden, normally drawn matches."""
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
    tool = str(ROOT/'build/native/native_content_tool.exe')
    project, store, source, image = (batch/name for name in ('project', 'store', 'owned.fms', 'portrait.png'))
    portrait(image)
    iso = args.iso.resolve()
    run(tool, 'clone-stock', iso, 'Bowser', project, 'Bowser CPU comparison')
    run(tool, 'import-parameters', project, ROOT/'examples/koopa_wip.fms', source)
    run(tool, 'script-build', project, source)
    package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
    run(tool, '--root', store, 'accept', package, iso)
    (batch/'package.txt').write_text(package)
    base = (ROOT/'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    cases = [('idle-final-destination-level9', 'final-destination', 9, 'cpu', 'human:zelda', 'idle'),
             ('shield-final-destination-level9', 'final-destination', 9, 'cpu', 'human:zelda', 'shield'),
             ('idle-final-destination-level4', 'final-destination', 4, 'cpu', 'human:zelda', 'idle'),
             ('idle-battlefield-level9', 'battlefield', 9, 'cpu', 'human:zelda', 'idle'),
             ('threat-bomb', 'final-destination', 9, 'human', 'cpu:zelda', 'bomb'),
             ('threat-up-smash', 'final-destination', 9, 'human', 'cpu:zelda', 'smash')]
    results = []
    for name, stage, level, p1, p2, response in cases:
        if args.case and name not in args.case: continue
        frames = bytearray()
        for tick in range(2200):
            sample = bytearray(base[24:104])
            if response == 'shield' and tick >= 120:
                struct.pack_into('<Hbb', sample, 24, 1 << 6, 0, 0)
            if response in ('bomb', 'smash'):
                buttons, x, y = 0, 0, 0
                if 85 <= tick < 142: x = 100
                if tick >= 230 and tick % 130 == 100:
                    buttons, y = (1 << 9 if response == 'bomb' else 1 << 8), (-80 if response == 'bomb' else 100)
                struct.pack_into('<Hbb', sample, 8, buttons, x, y)
            frames.extend(sample)
        inputs, directory = batch/(name+'.fmr'), batch/name
        inputs.write_bytes(base[:24]+frames)
        command = [sys.executable, str(ROOT/'tools/run_fighter_parity.py'), '--iso', str(iso),
                   '--inputs', str(inputs), '--stock-fighter', 'bowser', '--package-id', package,
                   '--content-root', str(store), '--stocks', '4', '--out-dir', str(directory),
                   '--stage', stage, '--p1-type', p1, '--cpu-level', str(level), '--p2', p2]
        if response in ('idle', 'shield'): command += ['--expect-damage-port', '2']
        if response == 'bomb': command += ['--expect-motion', '361']
        if response == 'smash': command += ['--expect-motion', '63']
        print('Comparing Bowser CPU: '+name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        counts = {}
        for label in ('stock', 'clone'):
            log = directory/(name+'-'+label+'.log')
            if log.exists():
                motions = re.findall(r'^\[action\] sample \d+ P1 .*?motion=(\d+) ',
                                     log.read_text(errors='replace'), re.M)
                counts[label] = {motion: motions.count(motion) for motion in sorted(set(motions), key=int)}
        results.append(dict(case=name, package=package, exit=code, motion_counts=counts))
        (batch/'results.json').write_text(json.dumps(results, indent=2)+'\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
