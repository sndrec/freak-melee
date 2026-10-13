"""Actual alternate/full-companion family preparation/publication; no actors or renderer."""
import argparse, hashlib, json, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'draft', 'portrait', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--companion-draft', type=Path, required=True)
    parser.add_argument('--companion-portrait', type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.drive.upper() != 'A:' or output.exists():
        parser.error('Use a fresh private output directory on A:')
    output.mkdir(parents=True)
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    companion_portrait = args.companion_portrait or args.portrait
    canonical = [ROOT / 'examples/popo_full.fms', ROOT / 'examples/nana_full.fms']
    companion_sources = [args.companion_draft / 'behavior.fms', args.companion_draft / 'partner/behavior.fms']
    if any(actual.read_bytes() != expected.read_bytes() for actual, expected in zip(companion_sources, canonical)):
        parser.error('Companion input must contain unchanged current full Popo/Nana sources')
    files = ([p for root in (args.draft, args.companion_draft) for p in root.rglob('*') if p.is_file()]
             + [args.exe, args.portrait, companion_portrait, Path(__file__), Path(__file__).with_name('test.c'),
                Path(__file__).with_name('README.md')] + canonical)
    files += list((ROOT / 'src').rglob('*.h')) + list((ROOT / 'extern/dolphin/include').rglob('*.h'))
    files += list((ROOT / 'src/melee/ft').glob('native*.c'))
    files += [ROOT / name for name in ('src/melee/pl/player.c', 'src/melee/gm/native_fighter_package.c',
        'src/melee/gm/native_content_store.c', 'src/melee/ft/ftcommon_assets.c', 'cmake/NativeSourcePackageTests.cmake')]
    inputs = {str(p.resolve()): sha(p) for p in files}
    command = list(map(str, [args.exe, args.iso, args.draft, args.portrait, output / 'cache',
                            args.companion_draft, companion_portrait]))
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                            timeout=240, creationflags=subprocess.CREATE_NO_WINDOW)
    (output / 'stdout.log').write_text(result.stdout)
    (output / 'stderr.log').write_text(result.stderr)
    unchanged = all(sha(Path(path)) == value for path, value in inputs.items())
    receipt = {'exit': result.returncode, 'command': command, 'inputs': inputs,
        'inputs_unchanged': unchanged,
        'scope': 'Actual alternate and unchanged full Popo/Nana private family packets/cache, DAT/resource source preparation, independent programs/banks/CPU profiles/tables and Player registration, mixed-family publication, both leases, failed source/CPU staging isolation, retained Windows page-history barrier and whole-family retirement.',
        'limits': ['Distinct activation hooks are prepared but no actors or transform callbacks run here.',
                   'Full companion source package publication is proved; no companion actor construction, authored startup, scheduling, game, graphics or whole-character gameplay acceptance.',
                   'The one-byte Windows page-history probe tests retirement admission, not aggregate game snapshot correctness.']}
    if result.returncode == 0:
        receipt['result'] = json.loads(result.stdout)
        receipt['status'] = 'PASS' if unchanged else 'FAIL changed inputs'
    else:
        receipt['status'] = 'FAIL'
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(result.stdout + result.stderr)
    print(json.dumps({'receipt': str(output / 'receipt.json'), 'status': receipt['status']}))
    raise SystemExit(result.returncode or (0 if unchanged else 1))

if __name__ == '__main__':
    main()
