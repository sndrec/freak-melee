"""Compare authored command-grab decisions and both fighters' live state."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import sys
from fighter_script_bundle_roundtrip import run, portrait, bundled_id

ROOT = Path(__file__).resolve().parents[2]


def recording(path, mode, facing, decision, contact):
    base = (ROOT/'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    frames = bytearray()
    start = 230 if contact else 170 if mode == 'air' else 150
    for tick in range(700):
        sample = bytearray(base[24:104])
        buttons, x, y = 0, 0, 0
        if not contact and facing == 'left' and 100 <= tick < 102: x = -40
        # The stock airborne catch bubble is much narrower than the grounded
        # one. Keep the target at its roughly 9.375-unit forward offset rather than letting
        # the lunge carry Bowser underneath it into the attacking capsules.
        if contact and 85 <= tick < (142 if mode == 'air' else 151): x = 100
        if contact and facing == 'left' and 200 <= tick < 202: x = -40
        if mode == 'air' and start - 25 <= tick < start - 17: buttons = 1 << 10
        if contact and mode == 'air' and start - 4 <= tick < start + 4:
            struct.pack_into('<Hbb', sample, 24, 1 << 10, 0, 0)
        if tick == start: buttons, x = 1 << 9, 80 if facing == 'right' else -80
        if contact and decision == 'bite' and tick in (270, 310, 350): buttons = 1 << 9
        throw_tick = 420 if decision == 'bite' else 310
        if mode == 'air' and decision in ('forward', 'back'): throw_tick = start + 22
        if contact and decision in ('forward', 'back', 'bite') and throw_tick <= tick < throw_tick + 2:
            x = (80 if facing == 'right' else -80) * (-1 if decision == 'back' else 1)
        if contact and decision == 'mash' and 270 <= tick < 380:
            pressed = 1 << 8 if tick % 5 == 0 else 0
            struct.pack_into('<Hbb', sample, 24, pressed, 80 if tick % 4 < 2 else -80, 0)
        struct.pack_into('<Hbb', sample, 8, buttons, x, y)
        frames.extend(sample)
    path.write_bytes(base[:24] + frames)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--out-dir', required=True, type=Path)
    parser.add_argument('--case', action='append')
    args = parser.parse_args()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    project, store, source, image = (batch/name for name in ('project', 'store', 'owned.fms', 'portrait.png'))
    tool = str(ROOT/'build/native/native_content_tool.exe')
    portrait(image)
    run(tool, 'clone-stock', args.iso, 'Bowser', project, 'Bowser capture comparison')
    run(tool, 'import-parameters', project, ROOT/'examples/koopa_wip.fms', source)
    run(tool, 'script-build', project, source)
    package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, args.iso))
    run(tool, '--root', store, 'accept', package, args.iso)
    (batch/'package.txt').write_text(package)
    cases = [(f'{mode}-{facing}-miss', mode, facing, 'wait', False)
             for mode in ('ground', 'air') for facing in ('right', 'left')]
    cases += [(f'{mode}-{decision}', mode, 'right', decision, True)
              for mode in ('ground', 'air') for decision in ('forward', 'back', 'wait', 'bite', 'mash')]
    results = []
    for name, mode, facing, decision, contact in cases:
        if args.case and name not in args.case: continue
        print('Comparing Bowser capture: ' + name, flush=True)
        inputs = batch/(name + '.fmr')
        recording(inputs, mode, facing, decision, contact)
        command = [sys.executable, str(ROOT/'tools/run_fighter_parity.py'), '--iso', str(args.iso),
                   '--inputs', str(inputs), '--stock-fighter', 'bowser', '--package-id', package,
                   '--content-root', str(store), '--stocks', '2', '--out-dir', str(batch/name),
                   '--expect-motion', '347' if mode == 'ground' else '353']
        if contact:
            command += ['--expect-motion', '348' if mode == 'ground' else '354']
            if decision == 'bite': command += ['--expect-motion', '349']
            if decision in ('forward', 'bite'): command += ['--expect-motion', '351' if mode == 'ground' or decision == 'bite' else '357']
            if decision == 'back': command += ['--expect-motion', '352' if mode == 'ground' else '358']
            if decision == 'mash':
                command += ['--expect-port-motion', '1', '218', '--expect-port-motion', '2', '229']
            if decision != 'wait': command += ['--expect-damage-port', '2']
        code = subprocess.run(command, cwd=ROOT).returncode
        results.append(dict(case=name, package=package, exit=code))
        (batch/'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
