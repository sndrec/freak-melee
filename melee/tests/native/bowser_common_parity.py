"""Compare Bowser's common throw-pose and held-item policies with stock."""
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
    project, store, source, image = (batch/name for name in ('project', 'store', 'owned.fms', 'portrait.png'))
    portrait(image)
    iso = args.iso.resolve()
    run(tool, 'clone-stock', iso, 'Bowser', project, 'Bowser common comparison')
    run(tool, 'import-parameters', project, ROOT/'examples/koopa_wip.fms', source)
    run(tool, 'script-build', project, source)
    package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
    run(tool, '--root', store, 'accept', package, iso)
    (batch/'package.txt').write_text(package)
    base = (ROOT/'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes()
    results = []
    for name in ('throw-down-zelda', 'throw-down-peach', 'throw-down-mario', 'bat-fortress'):
        if args.case and name not in args.case: continue
        frames = bytearray()
        throw = name.startswith('throw-')
        opponent = name.removeprefix('throw-down-') if throw else 'zelda'
        for tick in range(700):
            sample = bytearray(base[24:104])
            buttons, x, y = 0, 0, 0
            if throw:
                if 85 <= tick < 151: x = 100
                if tick == 200: buttons = 16
                # Stick alone selects the common throw; A would pummel first.
                if 220 <= tick < 225: y = -100
            else:
                if tick == 220: buttons = 1 << 8
                if tick == 280: buttons, y = 1 << 9, 80
                if tick == 500: buttons = 16
            struct.pack_into('<Hbb', sample, 8, buttons, x, y)
            frames.extend(sample)
        inputs, directory = batch/(name+'.fmr'), batch/name
        inputs.write_bytes(base[:24]+frames)
        command = [sys.executable, str(ROOT/'tools/run_fighter_parity.py'), '--iso', str(iso),
                   '--inputs', str(inputs), '--stock-fighter', 'bowser', '--package-id', package,
                   '--content-root', str(store), '--stocks', '2', '--out-dir', str(directory),
                   '--p2', 'human:'+opponent]
        if throw:
            command += ['--expect-motion', '222', '--expect-damage-port', '2',
                        '--expect-port-motion', '2', '243' if opponent in ('peach', 'zelda') else '242']
        else:
            command += ['--fixture-item', '11,120,-55,10', '--expect-item-kind', '11',
                        '--expect-motion', '92', '--expect-motion', '359']
        print('Comparing Bowser common policy: '+name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        results.append(dict(case=name, package=package, exit=code))
        (batch/'results.json').write_text(json.dumps(results, indent=2)+'\n')
    return int(not results or any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
