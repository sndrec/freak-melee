"""Review an extracted release, local updates and rollback with isolated user data.

Uses the real friend launcher and native client. No release is uploaded or
downloaded from the hosted channel. Both prepared releases must be signed by the
same test publisher, with the second release issued after the first.
"""
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import secrets
import shutil
import subprocess
import time
import zipfile


def hashes(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}


def close_window(pid):
    user = ctypes.WinDLL('user32', use_last_error=True)
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    user.EnumWindows.argtypes = [callback_type, wintypes.LPARAM]
    found = []

    @callback_type
    def visit(window, unused):
        owner = wintypes.DWORD()
        user.GetWindowThreadProcessId(window, ctypes.byref(owner))
        if owner.value == pid:
            found.append(window)
            user.PostMessageW(window, 0x0010, 0, 0)  # WM_CLOSE, including hidden SDL windows.
        return True

    user.EnumWindows(visit, 0)
    assert found, f'No window owned by test child {pid}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['first', 'next', 'profile', 'control', 'iso', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    first, next_release = args.first.resolve(), args.next.resolve()
    first_manifest = json.loads((first/'latest.json').read_text())
    next_manifest = json.loads((next_release/'latest.json').read_text())
    a, b = first_manifest['release'], next_manifest['release']
    assert a != b and next_manifest['issued_ms'] > first_manifest['issued_ms']
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    install = output/'Friend installation with spaces'
    with zipfile.ZipFile(first/f'freak-melee-setup-{a}.zip') as archive:
        archive.extractall(install)
    appdata = output/'appdata'
    shutil.copytree(args.profile.resolve(), appdata)
    data = appdata/'Freak Melee'
    assert (data/'css-layout.bin').is_file()
    assert list((data/'content/installed').glob('*.fmi'))
    assert (data/'content/hub-profile.txt').is_file()
    identity = 'release-review-'+secrets.token_hex(6)
    windows = Path(os.environ['SystemRoot'])
    powershell = windows/'System32/WindowsPowerShell/v1.0/powershell.exe'
    env = dict(os.environ, LOCALAPPDATA=str(appdata), FREAK_MELEE_CONTROL_ID=identity,
               FREAK_MELEE_CHAT_ENDPOINT='http://127.0.0.1:1',
               FREAK_MELEE_COMMUNITY_ENDPOINT='http://127.0.0.1:1',
               FREAK_MELEE_SERVICE_ENDPOINT='http://127.0.0.1:1',
               PATH=str(windows/'System32')+';'+str(windows))
    flags = subprocess.CREATE_NO_WINDOW
    launcher = install/'freak_launcher.ps1'
    commands = [str(powershell), '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(launcher)]
    launch_log = output/'launch.log'
    game_pid = None
    with launch_log.open('w') as log:
        process = subprocess.Popen(commands+['-Channel', str(first/'latest.json'),
                                    '-Iso', str(args.iso.resolve()), '-Hidden', '-Mute'],
                                   env=env, cwd=install, stdout=log, stderr=log,
                                   creationflags=flags)
        try:
            deadline = time.monotonic()+60
            while time.monotonic() < deadline:
                assert process.poll() is None, launch_log.read_text(errors='replace')[-3000:]
                if 'Native control channel ready' in launch_log.read_text(errors='replace'):
                    break
                time.sleep(.1)
            else:
                raise RuntimeError('Launcher did not start the native client')
            query = (f'Get-CimInstance Win32_Process -Filter "ParentProcessId = {process.pid}" | '
                     'Select-Object ProcessId,ExecutablePath | ConvertTo-Json -Compress')
            children = json.loads(subprocess.check_output(
                [str(powershell), '-NoProfile', '-Command', query], env=env,
                creationflags=flags, text=True))
            if isinstance(children, dict):
                children = [children]
            expected_game = (install/'versions'/a/'melee_native.exe').resolve()
            games = [c for c in children if c.get('ExecutablePath') and
                     Path(c['ExecutablePath']).resolve() == expected_game]
            assert len(games) == 1, children
            game_pid = games[0]['ProcessId']
            deadline = time.monotonic()+60
            while time.monotonic() < deadline:
                subprocess.run([str(args.control.resolve()), '--id', identity,
                                'ui-inspect', 'nav-5'], env=env, check=True,
                               capture_output=True, creationflags=flags)
                if '[hub-inspect] nav-5 origin=' in launch_log.read_text(errors='replace'):
                    break
                assert process.poll() is None
                time.sleep(.25)
            else:
                raise RuntimeError('Launched client did not reach the hub')
            close_window(game_pid)
            assert process.wait(timeout=30) == 0, launch_log.read_text(errors='replace')[-3000:]
            assert 'Update unavailable:' not in launch_log.read_text(errors='replace')
        finally:
            if process.poll() is None:
                if game_pid is not None:
                    subprocess.run(['taskkill.exe', '/PID', str(game_pid), '/T', '/F'],
                                   env=env, capture_output=True, creationflags=flags)
                process.terminate()
                process.wait(timeout=15)
    assert Path((data/'iso-path.txt').read_text()) == args.iso.resolve()
    baseline = hashes(data)

    def check(*parameters):
        # Exercise the batch entry point as well as the PowerShell launch above.
        command = '"'+str(install/'Freak Melee.bat')+'" '+subprocess.list2cmdline(list(parameters))
        result = subprocess.run(command, shell=True, env=env, cwd=install,
                                capture_output=True, text=True, creationflags=flags, timeout=120)
        with (output/'updates.log').open('a') as log:
            log.write(result.stdout+result.stderr)
        assert result.returncode == 0, result.stdout+result.stderr
        assert hashes(data) == baseline, 'Update or rollback changed player data'
        return json.loads((install/'state.json').read_text())

    state = check('-Channel', str(next_release/'latest.json'), '-CheckOnly')
    assert state == dict(current=b, previous=a, blocked=None, latest_ms=next_manifest['issued_ms']), state
    installed = install/'versions'/b
    for entry in next_manifest['files']:
        path = installed/entry['path']
        assert path.stat().st_size == entry['size']
        assert hashlib.sha256(path.read_bytes()).hexdigest() == entry['sha256']
    assert launcher.read_bytes() == (installed/'freak_launcher.ps1').read_bytes()
    state = check('-Rollback', '-CheckOnly')
    expected_rollback = dict(current=a, previous=b, blocked=b, latest_ms=next_manifest['issued_ms'])
    assert state == expected_rollback, state
    state = check('-Channel', str(next_release/'latest.json'), '-CheckOnly')
    assert state == expected_rollback, state
    assert 'Skipping rolled-back release' in (output/'updates.log').read_text()
    assert not list(install.glob('*.tmp'))
    (output/'passed.txt').write_text(
        f'PASS: extracted launcher boot and graceful close; {a} -> {b} update; rollback; '
        f'rejected version skipped; {len(baseline)} player-data files unchanged.\n')
    print((output/'passed.txt').read_text(), end='')


if __name__ == '__main__':
    main()
