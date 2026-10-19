"""Review listed/unlisted lobbies with two isolated, hidden, muted native clients."""
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
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--port', type=int, default=8795)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    root = Path(__file__).resolve().parents[2]
    service_root = root / 'melee/services/signaling'
    endpoint = f'http://127.0.0.1:{args.port}'
    clients, checks, screenshots = [], [], []
    with (output / 'service.log').open('w') as log:
        service = subprocess.Popen(['node', str(service_root / 'node_modules/wrangler/bin/wrangler.js'),
            'dev', '--local', '--ip', '127.0.0.1', '--port', str(args.port),
            '--persist-to', str(output / 'service-state')], cwd=service_root,
            stdout=log, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW)
        (output / 'service-pid.txt').write_text(str(service.pid))
        try:
            for _ in range(100):
                if service.poll() is not None: raise RuntimeError('Local service exited')
                try:
                    with urllib.request.urlopen(endpoint + '/health', timeout=.5) as response:
                        if response.status == 200: break
                except OSError: time.sleep(.2)
            else: raise RuntimeError('Local service did not start')
            os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = endpoint
            for name in ['Twilight', 'Guest']:
                client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                    output / name.lower(), name, window_size=(1280, 960))
                clients.append(client)
                client.click('nav-1')
            host, guest = clients

            def wait(client, element, pattern, timeout=25):
                deadline = time.monotonic() + timeout
                while time.monotonic() < deadline:
                    text = client.inspect(element)
                    if re.search(pattern, text): return text
                    time.sleep(.2)
                raise RuntimeError(f'{element}: {pattern!r} not found: {text}')

            wait(guest, 'hub-lobbies', r'connected=1 pending=0 count=0')
            host.click('create-lobby')
            host.enter('lobby-title', '   ')
            host.click('confirm-create-lobby')
            host.element('confirm-create-lobby')
            checks.append('Blank lobby name stays in the create screen')
            host.enter('lobby-title', "Twilight's test lobby")
            screenshots.append(str(host.screenshot()))
            host.click('local-players-2')
            host.click('confirm-create-lobby')
            host.element('change-listing')
            guest.click('refresh-lobbies')
            listed = wait(guest, 'hub-lobbies', r'title=Twilight.s test lobby.*fighting=2')
            checks.append('Named lobby appears with two actual local fighting seats')
            screenshots.append(str(guest.screenshot()))
            guest.click('join-listed-0')
            host.element('lobby-player')
            wait(guest, 'hub-lobbies', r'connected=1')
            guest.element('leave-lobby')
            checks.append('Directory row joins the real lobby without typing its code')
            screenshots.append(str(guest.screenshot()))
            guest.click('leave-lobby')
            guest.element('refresh-lobbies')
            host.click('change-listing')
            time.sleep(.7)
            guest.click('refresh-lobbies')
            wait(guest, 'hub-lobbies', r'pending=0 count=0 total=0')
            checks.append('Unlisting removes the lobby from the browser')
            code = re.search(r'code=([A-Z0-9]+)', listed).group(1)
            guest.enter('lobby-code', code)
            guest.click('join-lobby')
            guest.element('leave-lobby')
            checks.append('Unlisted lobby still accepts its code')
            guest.click('leave-lobby')
            guest.element('refresh-lobbies')
            host.click('leave-lobby')
            host.element('refresh-lobbies')
            time.sleep(.7)
            guest.click('refresh-lobbies')
            wait(guest, 'hub-lobbies', r'pending=0 count=0 total=0')
            checks.append('Empty lobby stays absent')
            receipt = {'checks': checks, 'screenshots': screenshots, 'endpoint': endpoint,
                       'exe': str(args.exe.resolve()), 'public_service_deployed': False}
            with args.exe.open('rb') as binary:
                receipt['exe_sha256'] = hashlib.file_digest(binary, 'sha256').hexdigest()
            (output / 'receipt.json').write_text(json.dumps(receipt, indent=2))
            print(json.dumps(receipt, indent=2))
        finally:
            for client in clients: client.stop()
            if service.poll() is None:
                subprocess.run(['taskkill', '/PID', str(service.pid), '/T', '/F'], capture_output=True,
                               creationflags=subprocess.CREATE_NO_WINDOW)
                service.wait(timeout=10)


if __name__ == '__main__':
    main()
