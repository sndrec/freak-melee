"""Actual collision algorithms and explicit collision/class/ObjAlloc/OS restore."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        parser.error('fresh A: output required')
    out.mkdir(parents=True)
    frozen = out / 'snapshot'
    build = ROOT / 'build/native'
    ninja = (build / 'build.ninja').read_text()
    begin = ninja.index('build native_source_assets_test.exe:')
    block = ninja[begin:ninja.index('\n\n', begin)]
    libraries = re.search(r'^  LINK_LIBRARIES = (.+)$', block, re.M).group(1).split()
    pairs = []
    for directory, target in [(ROOT/'src', frozen/'src'),
                              (ROOT/'extern/dolphin/include', frozen/'dolphin'),
                              (ROOT.parent/'aurora/include', frozen/'aurora')]:
        pairs += [(p, target/p.relative_to(directory)) for p in directory.rglob('*') if p.is_file()]
    pairs += [(HERE/p, frozen/p) for p in ['test.c', 'run.py', 'README.md']]
    pairs += [(build/p, frozen/'lib'/p) for p in dict.fromkeys(p for p in libraries if not p.startswith('-'))]
    pairs += [(p, out/p.name) for p in build.glob('*.dll')]
    pairs += [(build/'melee_native.exe', frozen/'melee_native.exe'),
              (build/'build.ninja', frozen/'build.ninja')]
    original = {str(p.resolve()): sha(p) for p, _ in pairs}
    for source, target in pairs:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    pins = {str(p.resolve()): sha(p) for _, p in pairs}
    assert all(original[str(a.resolve())] == pins[str(b.resolve())] for a, b in pairs)
    receipt = dict(status='RUNNING', snapshot_sha256=pins, original_sha256=original,
                   compiler_sha256=sha(args.compiler), commands=[],
                   scope='Actual production collision/load/query/island/attribute and snapshot algorithms, real JObj matrices, class/ObjAlloc/OS companion restore.',
                   limits=['Synthetic three-joint map and manually supplied model binding; not a production stage load or complete match snapshot.',
                           'No game window, renderer, audio, networking or performance acceptance.',
                           'Registry roots borrow class/ObjAlloc graphs; aggregate peer-candidate closure remains required.'])
    path = out/'receipt.json'
    path.write_text(json.dumps(receipt, indent=2)+'\n')
    command = [str(args.compiler.resolve()), '-std=gnu11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
               '-Wno-unused-function', '-Wno-array-parameter', '-fno-zero-initialized-in-bss',
               '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES',
               '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt', '-fuse-ld=lld', '-Wl,/opt:ref',
               '-I'+str(frozen/'src'), '-I'+str(frozen/'dolphin'), '-I'+str(frozen/'aurora'),
               str(frozen/'test.c')]
    command += [p if p.startswith('-') else str(frozen/'lib'/p) for p in libraries]
    command += ['-o', str(out/'test.exe')]
    try:
        for label, arguments in [('compile', command), ('fixture', [str(out/'test.exe')])]:
            result = subprocess.run(arguments, cwd=out, capture_output=True, text=True,
                                    timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
            (out/(label+'.log')).write_text(result.stdout+result.stderr, encoding='utf-8')
            receipt['commands'].append(dict(stage=label, command=arguments, exit=result.returncode))
            path.write_text(json.dumps(receipt, indent=2)+'\n')
            if result.returncode:
                raise RuntimeError(label+' failed: '+(result.stdout+result.stderr)[-3000:])
        receipt['summary'] = json.loads(result.stdout.strip().splitlines()[-1])
        receipt['snapshot_unchanged'] = all(sha(p) == value for p, value in pins.items())
        receipt['live_inputs_match_snapshot'] = all(sha(p) == value for p, value in original.items())
        receipt['status'] = 'PASS' if receipt['summary']['status'] == 'PASS' and receipt['snapshot_unchanged'] and receipt['live_inputs_match_snapshot'] else 'FAIL'
    except Exception as error:
        receipt.update(status='FAIL', error=str(error))
    path.write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], summary=receipt.get('summary'), receipt=str(path))))
    if receipt['status'] != 'PASS':
        print(receipt.get('error', 'Input verification failed'))
    return int(receipt['status'] != 'PASS')

if __name__ == '__main__':
    raise SystemExit(main())
