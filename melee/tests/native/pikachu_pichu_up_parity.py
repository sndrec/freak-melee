"""Compare Quick Attack/Agility starts, dashes and recovery with stock, hidden."""
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
    tool = str(ROOT / 'build/native/native_content_tool.exe')
    iso = args.iso.resolve()
    base = (ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    # Initial stick holds through the startup. A second direction is presented
    # during recovery, rather than issuing another B press.
    cases = [('ground-up-right', False, 1, (0, 80), None, 'final-destination'),
             ('ground-up-left', False, -1, (0, 80), None, 'final-destination'),
             ('ground-horizontal-right', False, 1, (80, 0), None, 'final-destination'),
             ('ground-horizontal-left', False, -1, (-80, 0), None, 'final-destination'),
             ('air-diagonal-right', True, 1, (60, 60), None, 'final-destination'),
             ('air-diagonal-left', True, -1, (-60, 60), None, 'final-destination'),
             ('air-down', True, 1, (0, -80), None, 'final-destination'),
             ('second-dash', False, 1, (80, 0), (0, 80), 'final-destination'),
             ('ground-slope', False, 1, (80, 0), None, 'yoshis-story'),
             ('platform-down', True, 1, (0, -80), None, 'battlefield'),
             ('neutral-stick', False, 1, (0, 0), None, 'final-destination'),
             ('small-stick-left', False, 1, (-15, 0), None, 'final-destination'),
             ('air-horizontal', True, 1, (80, 0), None, 'final-destination'),
             ('forced-turn', False, 1, (80, 0), None, 'final-destination')]
    results = []
    for fighter in ('Pikachu', 'Pichu'):
        project, store = batch / fighter, batch / (fighter + '-store')
        source, image = batch / (fighter + '.fms'), batch / (fighter + '.png')
        portrait(image)
        run(tool, 'clone-stock', iso, fighter, project, fighter + ' up-special comparison')
        run(tool, 'import-parameters', project, ROOT / 'examples' / (fighter.lower() + '_wip.fms'), source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        for suffix, air, direction, stick, second, stage in cases:
            name = fighter.lower() + '-' + suffix
            if args.case and name not in args.case:
                continue
            start = 180 if air else 150
            frames = bytearray()
            for tick in range(700):
                sample = bytearray(base[24:104])
                buttons, x, y = 0, 0, 0
                if direction < 0 and 100 <= tick < 102:
                    x = -40
                if suffix == 'forced-turn' and 100 <= tick < 120:
                    x = 80
                if air and 145 <= tick < 153:
                    buttons = 1 << 10
                if tick == start:
                    buttons, y = 1 << 9, 80
                elif start < tick < start + 65:
                    x, y = second if second and tick >= start + 17 else stick
                struct.pack_into('<Hbb', sample, 8, buttons, x, y)
                if suffix == 'forced-turn':
                    walk_end = 114 if fighter == 'Pikachu' else 140
                    if 100 <= tick < walk_end:
                        struct.pack_into('<Hbb', sample, 24, 0, -30, 0)
                    if tick == start + 10:
                        struct.pack_into('<Hbb', sample, 24, 1 << 9, -80, 0)
                frames.extend(sample)
            inputs = batch / (name + '.fmr')
            inputs.write_bytes(base[:24] + frames)
            command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                       '--iso', str(iso), '--inputs', str(inputs),
                       '--stock-fighter', fighter.lower(), '--package-id', package,
                       '--content-root', str(store), '--stocks', '2',
                       '--p2', 'human:mario' if suffix == 'forced-turn' else 'human:zelda',
                       '--out-dir', str(batch / name), '--stage', stage,
                       '--expect-motion', '356' if air else '353',
                       '--expect-motion', '357' if air or stick[1] > 0 or suffix in ('ground-slope', 'neutral-stick', 'small-stick-left') else '354']
            print('Comparing ' + name, flush=True)
            code = subprocess.run(command, cwd=ROOT).returncode
            if suffix == 'forced-turn' and code == 0:
                # Do not count a Cape that misses as forced-turn coverage.
                for label in ('stock', 'clone'):
                    log = (batch / name / (name + '-' + label + '.log')).read_text(errors='replace')
                    if not any(' P1 motion=355 ' in line and ' facing=bf800000 ' in line
                               for line in log.splitlines()):
                        print('Cape did not turn the recovering fighter in ' + label)
                        code = 1
            results.append(dict(case=name, package=package, exit=code))
            (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
