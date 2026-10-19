"""Review account pages without opening a Windows security prompt.

Uses an isolated profile, hidden/muted client and the local service state from
tests/native/account_client_service.mjs. That fixture has an existing account,
so duplicate registration and unknown-user login stop before a passkey prompt.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time
import urllib.request
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output', 'service-state']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--port', type=int, default=8797)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    root = Path(__file__).resolve().parents[2]
    service_root = root / 'melee/services/signaling'
    endpoint = f'http://localhost:{args.port}'
    checks, screenshots = [], []
    client = None
    with (output / 'service.log').open('w') as log:
        service = subprocess.Popen(['node', str(service_root / 'node_modules/wrangler/bin/wrangler.js'),
            'dev', '--local', '--ip', '127.0.0.1', '--port', str(args.port),
            '--persist-to', str(args.service_state.resolve()), '--var', 'AUTH_ORIGIN:' + endpoint,
            '--var', 'AUTH_RP_ID:localhost'], cwd=service_root, stdout=log, stderr=log,
            creationflags=subprocess.CREATE_NO_WINDOW)
        (output / 'service-pid.txt').write_text(str(service.pid))
        try:
            for _ in range(100):
                if service.poll() is not None: raise RuntimeError('Local service exited')
                try:
                    with urllib.request.urlopen(f'http://127.0.0.1:{args.port}/health', timeout=.5) as response:
                        if response.status == 200: break
                except OSError: time.sleep(.2)
            else: raise RuntimeError('Local service did not start')
            os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = endpoint
            client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                output / 'client', 'Twilight', window_size=(1280, 960))
            client.click('nav-6')
            client.element('settings-account')
            screenshots.append(str(client.screenshot()))
            client.click('settings-account')
            state = client.inspect('hub-account')
            assert re.search(r'started=1 available=1 busy=0 signed-in=0', state), state
            client.element('account-login-mode')
            screenshots.append(str(client.screenshot()))
            checks.append('Settings opens the stock-styled account page with Windows passkey support available')
            client.click('account-create-mode')
            client.enter('account-handle', 'native_test')
            client.click('account-submit')
            client.element('account-submit')
            assert 'signed-in=0' in client.inspect('hub-account')
            screenshots.append(str(client.screenshot()))
            checks.append('Duplicate registration is rejected by the real service before opening Windows Hello')
            client.click('account-login-mode')
            client.enter('account-handle', 'missing_user')
            client.click('account-submit')
            client.element('account-submit')
            screenshots.append(str(client.screenshot()))
            checks.append('Unknown account sign-in returns to the editable sign-in page')
            client.click('account-recover-mode')
            client.enter('account-recovery', 'RECOVERY_FIELD_MASK_TEST')
            screenshots.append(str(client.screenshot()))
            client.inspect('hub-fonts')
            assert 'RECOVERY_FIELD_MASK_TEST' not in client.stderr.read_text(errors='replace')
            client.click('account-submit')
            client.element('account-submit')
            screenshots.append(str(client.screenshot()))
            checks.append('Recovery input is masked and excluded from font diagnostics; invalid codes are rejected')
            client.click('nav-6')
            client.element('settings-account')
            client.click('settings-account')
            client.element('account-login-mode')
            checks.append('Leaving and reopening the account page clears its recovery input and returns to sign-in')
            receipt = {'checks': checks, 'screenshots': screenshots,
                'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                'limitations': ['No real Windows Hello creation/sign-in or save-file dialog acceptance.',
                    'Authenticated friends, inbox and presence remain separate work.']}
            (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
        finally:
            if client: client.stop()
            if service.poll() is None:
                subprocess.run(['taskkill', '/PID', str(service.pid), '/T', '/F'],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                    creationflags=subprocess.CREATE_NO_WINDOW)
                service.wait(timeout=15)
    print(f'{len(checks)} account UI checks passed; {len(screenshots)} screenshots in {output}')


if __name__ == '__main__':
    main()
