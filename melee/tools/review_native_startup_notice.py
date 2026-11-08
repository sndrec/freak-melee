"""Review startup account failures against a local HTTP fixture, hidden and muted."""
import argparse
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import threading
import time

from review_native_css_room import Client


class AccountConflict(BaseHTTPRequestHandler):
    registrations = 0

    def do_POST(self):
        self.rfile.read(int(self.headers.get('Content-Length', 0)))
        conflict = self.path == '/v1/accounts/taken_name/register/options'
        if conflict:
            AccountConflict.registrations += 1
        body = json.dumps({'error': 'That account name is already taken.' if conflict else 'Unexpected test request.'}).encode()
        self.send_response(409 if conflict else 400)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *_args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    empty = output / 'empty'; empty.mkdir()
    service = ThreadingHTTPServer(('127.0.0.1', 0), AccountConflict)
    threading.Thread(target=service.serve_forever, daemon=True).start()
    endpoint = f'http://localhost:{service.server_port}'
    previous = os.environ.get('FREAK_MELEE_SERVICE_ENDPOINT')
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = endpoint
    client = None
    screenshots = []

    def notice():
        for _ in range(40):
            text = client.inspect('hub-notice')
            if 'count=1' in text:
                assert 'Could not create account' in text and 'That account name is already taken.' in text, text
                time.sleep(.4)
                return
            time.sleep(.1)
        raise RuntimeError('Account failure did not open a notice')

    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), empty, output / 'client', 'Notice review',
            window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        client.click('setup-next')
        client.click('account-create-mode')
        client.enter('account-handle', 'taken_name')
        client.click('account-submit')
        notice(); screenshots.append(str(client.screenshot()))
        # B dismisses the notice, not the wizard's account page.
        client.command('hold', 0, 'B'); time.sleep(.8)
        assert 'count=0' in client.inspect('hub-notice')
        assert 'active=1 step=1' in client.inspect('startup')
        client.command('hold', 0, 'sx=0'); time.sleep(.2)
        # Clicking Continue underneath the notice only dismisses it.
        client.click('account-submit'); notice()
        client.click('account-overview')
        assert 'count=0' in client.inspect('hub-notice')
        assert 'active=1 step=1' in client.inspect('startup')
        # A second press after dismissal performs the requested navigation.
        client.click('account-overview')
        client.click('setup-next')
        client.enter('setup-name', 'Notice review')
        client.click('setup-next')
        assert 'active=1 step=3' in client.inspect('startup')
        assert AccountConflict.registrations == 2
        screenshots.append(str(client.screenshot()))
        (output / 'receipt.json').write_text(json.dumps({
            'checks': ['HTTP 409 message is shown as a modal startup notice',
                'Holding B dismisses once and stays on the account step',
                'A click over Continue dismisses without clicking through; a new click continues'],
            'screenshots': screenshots,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['Local HTTP error fixture; no production account or key data is used.',
                'Physical controller calibration needs the user hardware pass.'],
        }, indent=2) + '\n')
    finally:
        if client: client.stop()
        service.shutdown(); service.server_close()
        if previous is None: os.environ.pop('FREAK_MELEE_SERVICE_ENDPOINT', None)
        else: os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = previous
    print('3 startup notice checks passed')


if __name__ == '__main__':
    main()
