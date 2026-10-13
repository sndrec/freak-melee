"""Compare authored Thunder Jolt and both linked articles with stock, hidden."""
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
    cases = [('ground-right', False, 1, 'final-destination', 'zelda', False),
             ('ground-left', False, -1, 'final-destination', 'zelda', False),
             ('air-right', True, 1, 'final-destination', 'zelda', False),
             ('air-left', True, -1, 'final-destination', 'zelda', False),
             ('ground-slope', False, 1, 'yoshis-story', 'zelda', False),
             ('repeated', False, 1, 'final-destination', 'zelda', True),
             ('reflected', False, 1, 'final-destination', 'fox', False),
             ('absorbed', False, 1, 'final-destination', 'ness', False)]
    results = []
    for fighter, kind in (('Pikachu', 89), ('Pichu', 91)):
        project, store = batch / fighter, batch / (fighter + '-store')
        source, image = batch / (fighter + '.fms'), batch / (fighter + '.png')
        portrait(image)
        run(tool, 'clone-stock', iso, fighter, project, fighter + ' Thunder Jolt comparison')
        run(tool, 'import-parameters', project, ROOT / 'examples' / (fighter.lower() + '_wip.fms'), source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        for suffix, air, direction, stage, opponent, repeat in cases:
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
                if air and 145 <= tick < 153:
                    buttons = 1 << 10
                if tick == start or (repeat and tick in (250, 350)):
                    buttons = 1 << 9
                struct.pack_into('<Hbb', sample, 8, buttons, x, y)
                if opponent in ('fox', 'ness') and 120 <= tick < 280:
                    struct.pack_into('<Hbb', sample, 24, 1 << 9, 0, -80)
                frames.extend(sample)
            inputs = batch / (name + '.fmr')
            inputs.write_bytes(base[:24] + frames)
            command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                       '--iso', str(iso), '--inputs', str(inputs),
                       '--stock-fighter', fighter.lower(), '--package-id', package,
                       '--content-root', str(store), '--stocks', '2', '--p2', 'human:' + opponent,
                       '--out-dir', str(batch / name), '--stage', stage,
                       '--expect-motion', '342' if air else '341',
                       '--expect-item-kind', str(kind), '--expect-item-kind', str(kind + 1)]
            if suffix == 'reflected':
                command += ['--expect-item-owner-transfer', str(kind + 1), '1', '2']
            if suffix == 'absorbed':
                command += ['--expect-item-event', 'absorb', str(kind + 1)]
            print('Comparing ' + name, flush=True)
            code = subprocess.run(command, cwd=ROOT).returncode
            results.append(dict(case=name, package=package, exit=code))
            (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
