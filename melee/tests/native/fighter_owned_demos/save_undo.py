"""Actual CLI animation growth/save/undo and locked-file rollback on a private draft."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[3]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--tool', type=Path, default=ROOT / 'build/native/native_content_tool.exe')
    args = parser.parse_args()
    output = args.output.resolve()
    assert output.drive.upper() == 'A:' and not output.exists()
    output.mkdir(parents=True)
    project = output / 'project'
    shutil.copytree(args.source, project)
    env = os.environ.copy()
    for name in ('LOCALAPPDATA', 'APPDATA', 'TEMP', 'TMP'):
        directory = output / name.lower()
        directory.mkdir()
        env[name] = str(directory)
    receipt = {'status': 'RUNNING', 'commands': [], 'inputs': {
        str(args.tool.resolve()): sha(args.tool), str(Path(__file__).resolve()): sha(Path(__file__))}}

    def save():
        (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')

    def cli(stage, *words, expected=0):
        command = list(map(str, [args.tool.resolve(), '--root', output / 'store', *words]))
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True,
                                encoding='utf-8', errors='replace', timeout=120,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        (output / (stage + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
        receipt['commands'].append({'stage': stage, 'command': command, 'exit': result.returncode})
        save()
        assert result.returncode == expected, (stage, result.stdout, result.stderr)

    def pair():
        return tuple((project / name).read_bytes() for name in ('fighter.dat', 'animation.dat'))

    try:
        before_fighter, before_bank = pair()
        cli('grow', 'project-animation-insert', project, 0, 0, 1, 0)
        grown_fighter, grown_bank = pair()
        assert len(grown_bank) > len(before_bank) and grown_fighter != before_fighter
        cli('attribute-after-animation', 'project-attribute', project, 'gravity', '0.123')
        attribute_fighter, attribute_bank = pair()
        assert attribute_bank == grown_bank
        changed = [index for index, (a, b) in enumerate(zip(grown_fighter, attribute_fighter)) if a != b]
        assert 0 < len(changed) <= 4
        cli('undo-animation', 'project-animation-undo', project)
        expected_fighter = bytearray(before_fighter)
        for index in changed:
            expected_fighter[index] = attribute_fighter[index]
        assert pair() == (bytes(expected_fighter), before_bank), 'Undo must preserve later attribute edit'
        cli('redo-animation', 'project-animation-undo', project)
        assert pair() == (attribute_fighter, grown_bank)
        cli('undo-fighter', 'project-undo', project)
        assert pair() == (grown_fighter, grown_bank), 'Fighter undo must preserve current animation addresses'
        baseline = pair()
        # Hold a read handle that permits reads/writes but forbids rename/delete.
        # The CLI can stage both files, replace fighter.dat, then must roll it
        # back when the locked animation file cannot be replaced.
        windows = ctypes.WinDLL('kernel32', use_last_error=True)
        windows.CreateFileW.argtypes = [ctypes.c_wchar_p, ctypes.c_ulong, ctypes.c_ulong,
            ctypes.c_void_p, ctypes.c_ulong, ctypes.c_ulong, ctypes.c_void_p]
        windows.CreateFileW.restype = ctypes.c_void_p
        windows.CloseHandle.argtypes = [ctypes.c_void_p]
        handle = windows.CreateFileW(str(project / 'animation.dat'), 0x80000000, 3, None, 3, 0, None)
        assert handle not in (None, ctypes.c_void_p(-1).value)
        try:
            cli('locked-bank-failure', 'project-animation-insert', project, 0, 0, 2, 0, expected=1)
            assert pair() == baseline, 'Failed second replace left mismatched current files'
        finally:
            windows.CloseHandle(handle)
        assert not list(project.glob('*.tmp'))
        receipt.update(status='PASS actual CLI paired animation save/undo/redo and locked-bank rollback',
            growth_bytes=len(grown_bank) - len(before_bank), preserved_attribute_bytes=changed,
            limits=['Operation-level failure rollback; no power-loss/crash journal asserted.',
                    'Private Mario draft; memory/UI undo and demo key editing are separate fixtures.'])
    except Exception as error:
        receipt.update(status='FAIL', error=repr(error))
        save()
        raise
    save()
    print(receipt['status'])


if __name__ == '__main__':
    main()
