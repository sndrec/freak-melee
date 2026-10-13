"""Compare compiled PSI Magnet with unchanged original Ness C; no game launch."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

from ness_pkfire_stock_compare import root_offset

ROOT = Path(__file__).resolve().parents[2]
OFFSETS = (116, 120, 124, 132, 136, 140, 148, 152, 156, 160, 164, 168)


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
    attrs = 32 + struct.unpack_from('>I', data, root + 4)[0]
    raw = [(offset, struct.unpack_from('>I', data, attrs + offset)[0]) for offset in OFFSETS]
    (out / 'ness_magnet_raw.generated.h').write_text(
        'static const struct { unsigned offset; uint32_t bits; } raw_attrs[] = {\n' +
        ''.join('{%d,UINT32_C(0x%08x)},\n' % value for value in raw) + '};\n')
    generated = (project / 'behavior.generated.c').read_text()
    required = sorted(set(re.findall(r'host->(\w+) == 0', generated)))
    (out / 'ness_magnet_required_host.generated.h').write_text(''.join(
        'host.' + name + '=(__typeof__(host.' + name + '))unused_host;\n' for name in required))
    compiler = Path('C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe')
    flags = ['-std=gnu11', '-O2', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
             '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES',
             '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
             '-I' + str(sdk / 'src'), '-I' + str(sdk / 'extern/dolphin/include'),
             '-I' + str(out)]
    sources = [ROOT / 'tests/native/ness_magnet_stock_test.c',
               sdk / 'src/melee/ft/native_fighter_parameters.c',
               sdk / 'src/melee/ft/kinds/ftNess/ftnessspeciallw.c',
               sdk / 'src/melee/ft/native_fighter_damage.c']
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
    exe = out / 'ness_magnet_stock_test.exe'
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
