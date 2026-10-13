"""Compile real visibility import/load bodies with explicitly selected Clang."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[3])
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, output, compiler = args.root.resolve(), args.output.resolve(), args.compiler.resolve()
    output.mkdir(parents=True, exist_ok=True)
    subprocess.run([sys.executable, str(Path(__file__).with_name('fixture.py')),
                    '--root', str(root), '--output', str(output)], check=True)
    base = root / 'src/sysdolphin/baselib'
    units = [Path(__file__).with_name('runtime_test.c'),
             root / 'src/melee/ft/native_callback_lifetime.c']
    units += [base / (name + '.c') for name in ['dat', 'dat_import', 'dat_model',
              'dat_geometry', 'dat_material', 'dat_animation', 'dat_scene', 'dat_texture']]
    source_receipt = json.loads((output / 'source.json').read_text())
    inputs = units + [compiler, Path(__file__), Path(__file__).with_name('fixture.py'),
                      output / 'visibility_actual.inc']
    inputs += [root / name for name in source_receipt['inputs']]
    inputs += sorted((root / 'src').rglob('*.h'))
    inputs += sorted((root / 'extern/dolphin/include').rglob('*.h'))
    hashes = {str(path): digest(path) for path in inputs}
    exe = output / 'fighter_visibility_runtime_test.exe'
    command = [str(compiler), '-std=gnu11', '-O2', '-fuse-ld=lld',
               '-DMELEE_NATIVE=1', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS',
               '-I' + str(root / 'src'), '-I' + str(root / 'extern/dolphin/include'),
               '-I' + str(output), *map(str, units), '-o', str(exe)]
    logs = {}
    for name, cmd in [('compile', command), ('run', [str(exe)])]:
        result = subprocess.run(cmd, cwd=root, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        (output / (name + '.log')).write_text(result.stdout)
        if result.returncode:
            raise RuntimeError(f'{name} failed({result.returncode}): {result.stdout}')
        logs[name] = result.stdout.strip()
    for name, expected in hashes.items():
        if digest(Path(name)) != expected:
            raise RuntimeError('input changed during fixture: ' + name)
    receipt = {'result': logs['run'], 'inputs': hashes, 'command': command,
               'executable_sha256': digest(exe), 'scope': source_receipt['scope'],
               'compiler_sdk': 'Explicit compiler; all current production source/header inputs '
                               'hashed. This is not evidence from a frozen SDK or whole build.'}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'receipt': str(output / 'receipt.json'), 'result': logs['run']}))


if __name__ == '__main__':
    main()
