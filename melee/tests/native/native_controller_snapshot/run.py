"""Compile real controller/rumble snapshot operations with controlled devices."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        parser.error('fresh A: output required')
    out.mkdir(parents=True)
    frozen = out / 'snapshot'
    base = ROOT / 'src/sysdolphin/baselib'
    names = ['controller.c', 'rumble.c', 'rumble_output.c', 'native_controller_snapshot.c']
    pairs = [(base/name, frozen/'src/sysdolphin/baselib'/name) for name in names]
    pairs.append((ROOT/'src/melee/gm/native_match_arena.c', frozen/'src/melee/gm/native_match_arena.c'))
    pairs.append((ROOT/'src/melee/gm/native_feedback_input.c', frozen/'src/melee/gm/native_feedback_input.c'))
    for directory in [ROOT/'src', ROOT/'extern/dolphin/include']:
        for source in directory.rglob('*.h'):
            target = frozen / ('dolphin' if 'extern' in source.parts else 'src')
            relative = source.relative_to(ROOT/'extern/dolphin/include' if 'extern' in source.parts else ROOT/'src')
            pairs.append((source, target/relative))
    pairs += [(HERE/name, frozen/name) for name in ['run.py', 'test.c', 'README.md']]
    original = {str(source.resolve()): sha(source) for source, _ in pairs}
    for source, target in pairs:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    pins = {str(target.resolve()): sha(target) for _, target in pairs}
    flags = ['-std=c11', '-O2', '-ffp-contract=off', '-DMELEE_NATIVE=1',
             '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES',
             '-I'+str(frozen/'src'), '-I'+str(frozen/'dolphin')]
    units = [frozen/'src/sysdolphin/baselib'/name for name in names]
    units.insert(0, frozen/'src/melee/gm/native_match_arena.c')
    units.insert(0, frozen/'src/melee/gm/native_feedback_input.c')
    receipt = dict(status='RUNNING', original_sha256=original, snapshot_sha256=pins,
                   compiler_sha256=sha(args.compiler), commands=[], limits=[
                       'Controlled physical pad/motor and replay/profile hooks; actual queue, input conversion, rumble and snapshot algorithms.',
                       'No window, audio, full-match restore, command bank residency closure or performance acceptance.'])
    path = out/'receipt.json'
    path.write_text(json.dumps(receipt, indent=2)+'\n')
    try:
        strict = [str(args.compiler), *flags, '-Wall', '-Wextra', '-Werror',
                  '-Wno-array-parameter', '-fsyntax-only', str(units[-1]), str(frozen/'test.c')]
        compile_cmd = [str(args.compiler), *flags, '-fuse-ld=lld', *map(str, units),
                       str(frozen/'test.c'), '-o', str(out/'test.exe')]
        for stage, command in [('strict', strict), ('compile', compile_cmd), ('fixture', [str(out/'test.exe')])]:
            result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                                    timeout=120, creationflags=subprocess.CREATE_NO_WINDOW)
            (out/(stage+'.log')).write_text(result.stdout+result.stderr)
            receipt['commands'].append(dict(stage=stage, command=command, exit=result.returncode))
            path.write_text(json.dumps(receipt, indent=2)+'\n')
            if result.returncode:
                raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-3000:])
        receipt['summary'] = json.loads(result.stdout)
        assert receipt['summary']['status'] == 'PASS'
        assert all(sha(p) == value for p, value in pins.items())
        assert all(sha(p) == value for p, value in original.items())
        receipt['status'] = 'PASS'
    except Exception as error:
        receipt.update(status='FAIL', error=str(error))
    path.write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], summary=receipt.get('summary'),
                         error=receipt.get('error'), receipt=str(path))))
    return int(receipt['status'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
