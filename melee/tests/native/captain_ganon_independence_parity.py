"""Build independent Falcon/Ganon packages and compare hidden, muted drawn matches."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait, run

ROOT = Path(__file__).resolve().parents[2]
INPUTS = ROOT / 'tests/native/parity_inputs'


def neutral_recording(samples, taunt=False):
    template = (INPUTS / 'fighter-left-neutral-inputs.fmr').read_bytes()
    assert template[:8] == b'FMREPLAY' and len(template) >= 104
    frames = bytearray()
    for tick in range(samples):
        sample = bytearray(template[24:104])
        for port in range(4):
            struct.pack_into('<HbbbbBB', sample, 8 + port * 16, 0, 0, 0, 0, 0, 0, 0)
        if taunt and tick == 130:
            struct.pack_into('<H', sample, 8, 8)  # GameCube D-pad up.
        frames.extend(sample)
    return template[:24] + frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--out-dir', required=True, type=Path)
    parser.add_argument('--exe', type=Path, default=ROOT / 'build/native/melee_native.exe')
    parser.add_argument('--tool', type=Path, default=ROOT / 'build/native/native_content_tool.exe')
    parser.add_argument('--fighter', choices=('Captain', 'Ganon'), action='append')
    parser.add_argument('--case', choices=('left-neutral', 'side-ground', 'side-air',
                        'dive-catch', 'dive-interrupted', 'kick-wall',
                        'air-kick-hit', 'taunt', 'cpu'), action='append')
    args = parser.parse_args()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    iso, tool = args.iso.resolve(), str(args.tool.resolve())
    image = batch / 'portrait.png'
    portrait(image)
    store = batch / 'store'
    results = []
    for fighter in args.fighter or ('Captain', 'Ganon'):
        slug = fighter.lower()
        project, source = batch / slug, batch / (slug + '_owned.fms')
        template = ROOT / 'examples' / (slug + '_wip.fms')
        assert 'behavior independent' in template.read_text(), 'Template is not independent'
        stock_name = 'Captain Falcon' if fighter == 'Captain' else 'Ganondorf'
        base_name = stock_name if fighter == 'Captain' else 'Ganon'
        run(tool, 'clone-stock', iso, base_name, project, fighter + ' independence comparison')
        run(tool, 'import-parameters', project, template, source)
        run(tool, 'script-build', project, source)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
        (project / 'package.txt').write_text(package + '\n')
        cases = [
            ('left-neutral', 'fighter-left-neutral-inputs.fmr', 'final-destination', (347,), None),
            ('side-ground', slug + '-side-contact-inputs.fmr', 'final-destination', (349, 350), 2),
            ('side-air', slug + '-side-air-contact-inputs.fmr', 'final-destination', (351, 352), 2),
            ('dive-catch', 'fighter-dive-catch-inputs.fmr', 'final-destination', (353, 355, 356), 2),
            ('dive-interrupted', 'fighter-dive-interrupted-inputs.fmr', 'final-destination', (353, 80), 1),
            ('kick-wall', 'fighter-kick-wall-bounce-inputs.fmr', 'onett', (363,), None),
            ('air-kick-hit', 'fighter-air-kick-hit-inputs.fmr', 'final-destination', (359, 360), 2),
        ]
        taunt, cpu = batch / (slug + '-taunt.fmr'), batch / (slug + '-cpu.fmr')
        taunt.write_bytes(neutral_recording(440, taunt=True))
        cpu.write_bytes(neutral_recording(1800))
        cases += [('taunt', taunt, 'final-destination', (264,), None),
                  ('cpu', cpu, 'final-destination', (), None)]
        for name, inputs, stage, motions, damage in cases:
            if args.case and name not in args.case:
                continue
            inputs = inputs if isinstance(inputs, Path) else INPUTS / inputs
            output = batch / (slug + '-' + name)
            command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                       '--iso', str(iso), '--exe', str(args.exe.resolve()),
                       '--inputs', str(inputs), '--stock-fighter', stock_name,
                       '--package-id', package, '--content-root', str(store),
                       '--out-dir', str(output), '--stocks', '2', '--stage', stage,
                       '--independent']
            for motion in motions:
                command += ['--expect-motion', str(motion)]
            if damage:
                command += ['--expect-damage-port', str(damage)]
            if name == 'cpu':
                command += ['--p1-type', 'cpu']
            log = batch / (slug + '-' + name + '-result.log')
            with log.open('wb') as stream:
                result = subprocess.run(command, cwd=ROOT, stdout=stream,
                                        stderr=subprocess.STDOUT)
            exit_code = result.returncode
            if name == 'cpu' and exit_code == 0:
                # Equality is not useful CPU evidence when both sides stand idle.
                for label in ('stock', 'clone'):
                    trace = output / (inputs.stem + '-' + label + '.log')
                    states = re.findall(r'\[fighter\] sample (\d+) P1 .*?'
                                        r'stick=([0-9a-f]+),([0-9a-f]+)',
                                        trace.read_text())
                    active = any(int(tick) >= 60 and
                                 ((int(x, 16) | int(y, 16)) & 0x7fffffff)
                                 for tick, x, y in states)
                    if not active:
                        with log.open('a') as stream:
                            stream.write(label + ' CPU never produced movement; '
                                         'active CPU acceptance rejected.\n')
                        exit_code = 1
            row = dict(fighter=fighter, case=name, package=package,
                       inputs=str(inputs), input_sha256=hashlib.sha256(inputs.read_bytes()).hexdigest(),
                       exit_code=exit_code, log=str(log))
            results.append(row)
            print(f'{fighter} {name}: exit {exit_code}', flush=True)
            (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(any(row['exit_code'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
