"""Compare owned Skull Bash decisions with stock in hidden, drawn matches."""
import argparse
import json
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
    tool = str(ROOT/'build/native/native_content_tool.exe')
    iso = args.iso.resolve()
    base = (ROOT/'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    cases = [('ground-right-tap', False, 1, 1, False, 'final-destination'),
             ('ground-left-tap', False, -1, 1, False, 'final-destination'),
             ('air-right-tap', True, 1, 1, False, 'final-destination'),
             ('air-left-tap', True, -1, 1, False, 'final-destination'),
             ('ground-full-charge', False, 1, 240, False, 'final-destination'),
             ('ground-contact', False, 1, 1, True, 'final-destination'),
             ('ground-slope', False, 1, 35, False, 'yoshis-story')]
    results = []
    for fighter in ('Pikachu', 'Pichu'):
        project, store = batch/fighter, batch/(fighter+'-store')
        source, image = batch/(fighter+'.fms'), batch/(fighter+'.png')
        portrait(image)
        run(tool, 'clone-stock', iso, fighter, project, fighter+' Skull Bash comparison')
        run(tool, 'import-parameters', project, ROOT/'examples'/(fighter.lower()+'_wip.fms'), source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        for suffix, air, direction, hold, contact, stage in cases:
            name = fighter.lower()+'-'+suffix
            if args.case and name not in args.case: continue
            start = 240 if contact else 180 if air else 150
            frames = bytearray()
            for tick in range(700):
                sample = bytearray(base[24:104])
                buttons, x, y = 0, 0, 0
                if direction < 0 and 100 <= tick < 102: x = -40
                if contact and 85 <= tick < (120 if fighter == 'Pichu' else 114): x = 100
                if air and 145 <= tick < 153: buttons = 1 << 10
                if start <= tick < start + hold:
                    buttons = 1 << 9
                    if tick == start: x = direction * 80
                struct.pack_into('<Hbb', sample, 8, buttons, x, y)
                frames.extend(sample)
            inputs = batch/(name+'.fmr')
            inputs.write_bytes(base[:24]+frames)
            command = [sys.executable, str(ROOT/'tools/run_fighter_parity.py'), '--iso', str(iso),
                       '--inputs', str(inputs), '--stock-fighter', fighter.lower(),
                       '--package-id', package, '--content-root', str(store), '--stocks', '2',
                       '--out-dir', str(batch/name), '--stage', stage,
                       '--expect-motion', '348' if air else '343',
                       '--expect-motion', '352' if air else '347', '--expect-motion', '350']
            # On a tap, Hold enters and exits within one callback phase.
            # Require a visible Hold state only in the sustained recordings.
            if hold > 1: command += ['--expect-motion', '349' if air else '344']
            if contact: command += ['--expect-damage-port', '2']
            print('Comparing '+name, flush=True)
            code = subprocess.run(command, cwd=ROOT).returncode
            results.append(dict(case=name, package=package, exit=code))
            (batch/'results.json').write_text(json.dumps(results, indent=2)+'\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
