"""Exercise independent Mario/Dr. Mario in hidden, muted, drawn matches."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait, run
from captain_ganon_independence_parity import neutral_recording

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / 'tests/native/parity_inputs'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    parser.add_argument('--exe', type=Path, default=ROOT / 'build/native/melee_native.exe')
    parser.add_argument('--tool', type=Path, default=ROOT / 'build/native/native_content_tool.exe')
    parser.add_argument('--fighter', choices=('Mario', 'DrMario'), action='append')
    parser.add_argument('--case', choices=('neutral-ground', 'neutral-air', 'side-ground',
                        'side-air', 'up-ground', 'up-air', 'down-ground', 'down-air',
                        'taunt', 'cpu'), action='append')
    args = parser.parse_args()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    iso, tool, store = args.iso.resolve(), str(args.tool.resolve()), batch / 'store'
    image = batch / 'portrait.png'
    portrait(image)
    rows = []
    for fighter in args.fighter or ('Mario', 'DrMario'):
        slug = fighter.lower()
        project, source = batch / slug, batch / (slug + '_owned.fms')
        template = ROOT / 'examples' / (slug + '_full.fms')
        assert 'behavior independent' in template.read_text()
        run(tool, 'clone-stock', iso, fighter, project, fighter + ' independent comparison')
        run(tool, 'import-parameters', project, template, source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        (project / 'package.txt').write_text(package + '\n')
        cases = []
        for move, ground, air in (('neutral', 343, 344), ('side', 345, 346),
                                   ('up', 347, 348), ('down', 349, 350)):
            for posture, motion in (('ground', ground), ('air', air)):
                name = move + '-' + posture
                cases.append((name, INPUTS / ('drmario-special-' + name + '-inputs.fmr'), motion))
        taunt, cpu = batch / (slug + '-taunt.fmr'), batch / (slug + '-cpu.fmr')
        taunt.write_bytes(neutral_recording(440, taunt=True))
        cpu.write_bytes(neutral_recording(1800))
        cases += [('taunt', taunt, 341 if fighter == 'DrMario' else 264),
                  ('cpu', cpu, None)]
        for name, inputs, motion in cases:
            if args.case and name not in args.case:
                continue
            output, log = batch / (slug + '-' + name), batch / (slug + '-' + name + '-result.log')
            command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                       '--iso', str(iso), '--exe', str(args.exe.resolve()),
                       '--inputs', str(inputs), '--stock-fighter', fighter,
                       '--package-id', package, '--content-root', str(store),
                       '--out-dir', str(output), '--stocks', '1', '--independent']
            if motion is not None:
                command += ['--expect-motion', str(motion)]
            if name == 'cpu':
                command += ['--p1-type', 'cpu']
            with log.open('wb') as stream:
                result = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
            code = result.returncode
            if name == 'cpu' and code == 0:
                for label in ('stock', 'clone'):
                    trace = output / (inputs.stem + '-' + label + '.log')
                    states = re.findall(r'\[fighter\] sample (\d+) P1 .*?stick=([0-9a-f]+),([0-9a-f]+)', trace.read_text())
                    if not any(int(tick) >= 60 and ((int(x, 16) | int(y, 16)) & 0x7fffffff)
                               for tick, x, y in states):
                        with log.open('a') as stream:
                            stream.write(label + ' CPU never produced movement; rejected.\n')
                        code = 1
            rows.append(dict(fighter=fighter, case=name, package=package,
                             inputs=str(inputs), input_sha256=hashlib.sha256(inputs.read_bytes()).hexdigest(),
                             exit_code=code, log=str(log)))
            (batch / 'results.json').write_text(json.dumps(rows, indent=2) + '\n')
            print(f'{fighter} {name}: exit {code}', flush=True)
    return int(any(row['exit_code'] for row in rows))


if __name__ == '__main__':
    raise SystemExit(main())
