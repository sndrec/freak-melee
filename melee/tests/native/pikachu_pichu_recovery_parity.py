"""Compare real CPU recovery decisions from repeatable airborne placements."""
import argparse
import json
from pathlib import Path
import re
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
    inputs = batch / 'cpu-recovery.fmr'
    inputs.write_bytes(base[:24] + base[24:104] * 1000)
    results = []
    for fighter in ('Pikachu', 'Pichu'):
        project, store = batch / fighter, batch / (fighter + '-store')
        source, image = batch / (fighter + '.fms'), batch / (fighter + '.png')
        portrait(image)
        run(tool, 'clone-stock', iso, fighter, project, fighter + ' recovery comparison')
        run(tool, 'import-parameters', project, ROOT / 'examples' / (fighter.lower() + '_wip.fms'), source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        for distance, x, y in (('far', 175, -5), ('near', 110, -25)):
            for side, direction in (('left', -1), ('right', 1)):
                name = fighter.lower() + '-' + distance + '-' + side
                if args.case and name not in args.case:
                    continue
                directory = batch / name
                command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                           '--iso', str(iso), '--inputs', str(inputs),
                           '--stock-fighter', fighter.lower(), '--package-id', package,
                           '--content-root', str(store), '--stocks', '4', '--p1-type', 'cpu',
                           '--cpu-level', '9', '--out-dir', str(directory),
                           '--fixture-airborne', f'1,120,{x * direction},{y}',
                           '--expect-motion', '356', '--expect-motion', '357', '--expect-motion', '358']
                print('Comparing ' + name, flush=True)
                code = subprocess.run(command, cwd=ROOT).returncode
                traces = {}
                dash_entries = {}
                for label in ('stock', 'clone'):
                    log = directory / ('cpu-recovery-' + label + '.log')
                    if log.exists():
                        text = log.read_text(errors='replace')
                        traces[label] = re.findall(r'^\[cpu-recovery\] sample=(\d+) P1 from=([^\n]+)', text, re.M)
                        dash_entries[label] = len(re.findall(
                            r'^\[action\] sample (?:1[2-9]\d|2[0-4]\d) P1 .*?motion=357 ', text, re.M))
                if code == 0 and (not traces.get('stock') or traces.get('stock') != traces.get('clone')):
                    print('Recovery decision coverage missing or different: ' + name, flush=True)
                    code = 1
                if code == 0:
                    coordinates = re.match(r'([-\d.]+),[-\d.]+ target=([-\d.]+),', traces['stock'][0][1])
                    far = coordinates and abs(float(coordinates[1]) - float(coordinates[2])) > 60
                    if coordinates is None or far != (distance == 'far'):
                        print('Expected recovery distance branch not exercised: ' + name, flush=True)
                        code = 1
                    if min(dash_entries.values()) < 2:
                        print('Both recovery dashes were not exercised: ' + name, flush=True)
                        code = 1
                results.append(dict(case=name, package=package, exit=code,
                                    recovery_decisions=traces, initial_dash_entries=dash_entries))
                (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
