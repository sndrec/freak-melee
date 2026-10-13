"""Compare a compiled PK Fire candidate with original stock C; no game launch."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def root_offset(data, name):
    size, relocations, roots, externs = struct.unpack_from('>4I', data, 4)
    table = 32 + size + relocations * 4
    strings = table + (roots + externs) * 8
    for index in range(roots):
        target, offset = struct.unpack_from('>2I', data, table + index * 8)
        if data[strings + offset:].split(b'\0', 1)[0].decode() == name:
            return 32 + target
    raise AssertionError('Missing DAT root ' + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk-root', type=Path, required=True)
    parser.add_argument('--project', type=Path, required=True)
    parser.add_argument('--stock-dat', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    args = parser.parse_args()
    sdk, project, out = args.sdk_root.resolve(), args.project.resolve(), args.out_dir.resolve()
    for name in ('module.dll', 'fighter_parameters.fmp', 'behavior.generated.c'):
        if not (project / name).is_file():
            parser.error('Missing compiled candidate file: ' + str(project / name))
    if not args.stock_dat.is_file():
        parser.error('Missing raw stock DAT: ' + str(args.stock_dat))
    out.mkdir(parents=True, exist_ok=True)
    data = args.stock_dat.read_bytes()
    root = root_offset(data, 'ftDataNess')
    pointer = lambda at: 32 + struct.unpack_from('>I', data, at)[0]
    fighter = pointer(root + 4)
    articles = pointer(root + 72)
    raw = list(struct.unpack_from('>7f', data, fighter + 32))
    for slot, count in ((0, 2), (1, 3)):
        attrs = pointer(pointer(articles + slot * 4) + 4)
        raw.extend(struct.unpack_from('>' + str(count) + 'f', data, attrs))
    (out / 'ness_raw_tuning.generated.h').write_text(
        'static const float raw_tuning[] = {' + ','.join(v.hex() + 'f' for v in raw) + '};\n')
    generated = (project / 'behavior.generated.c').read_text()
    required = sorted(set(re.findall(r'host->(\w+) == 0', generated)))
    (out / 'ness_required_host.generated.h').write_text(''.join(
        'host.' + name + '=(__typeof__(host.' + name + '))unused_host;\n' for name in required))
    compiler = Path('C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe')
    flags = ['-std=gnu11', '-O2', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
             '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES',
             '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
             '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include'),
             '-I' + str(out)]
    sources = [ROOT / 'tests/native/ness_pkfire_stock_test.c',
               sdk / 'src/melee/ft/native_fighter_parameters.c',
               sdk / 'src/melee/ft/kinds/ftNess/ftnessspecials.c',
               sdk / 'src/melee/it/kinds/itnesspkfire.c',
               sdk / 'src/melee/it/kinds/itnesspkfirepillar.c']
    objects = []
    for index, source in enumerate(sources):
        obj = out / (str(index) + '.obj')
        result = subprocess.run([str(compiler), *flags, '-c', str(source), '-o', str(obj)],
                                capture_output=True, text=True)
        (out / (str(index) + '.compile.log')).write_text(result.stdout + result.stderr)
        if result.returncode:
            print(result.stdout + result.stderr)
            return result.returncode
        objects.append(str(obj))
    exe = out / 'ness_pkfire_stock_test.exe'
    result = subprocess.run([str(compiler), '-fuse-ld=lld', *objects, '-o', str(exe),
                             '-Wl,/OPT:REF'], capture_output=True, text=True)
    (out / 'link.log').write_text(result.stdout + result.stderr)
    if result.returncode:
        print(result.stdout + result.stderr)
        return result.returncode
    result = subprocess.run([str(exe), str(project / 'module.dll'),
                             str(project / 'fighter_parameters.fmp')], capture_output=True, text=True)
    (out / 'comparison.log').write_text(result.stdout + result.stderr)
    manifest = [*sources, args.stock_dat, project / 'module.dll',
                project / 'fighter_parameters.fmp', project / 'behavior.generated.c']
    (out / 'source_manifest.json').write_text(json.dumps({
        str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in manifest}, indent=2) + '\n')
    print(result.stdout + result.stderr)
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
