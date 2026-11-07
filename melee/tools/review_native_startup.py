"""Check first startup and resumption in an isolated, hidden, muted native game."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import time
from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    empty = output / 'empty'; empty.mkdir()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = 'http://localhost:1'
    client = None
    checks, screens = [], []

    def startup(step, timeout=30):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            text = client.inspect('startup')
            if re.search(rf'startup active=1 step={step}\b', text): return text
            time.sleep(.1)
        raise RuntimeError(f'Expected setup step {step}: {text}')

    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), empty, output / 'first', 'Startup review',
            window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        startup(0); screens.append(str(client.screenshot()))
        client.click('setup-next'); startup(1)
        client.enter('setup-name', 'Startup review'); client.enter('setup-bio', 'A new player.')
        screens.append(str(client.screenshot()))
        client.click('setup-next'); startup(2)
        screens.append(str(client.screenshot()))
        client.click('setup-next'); startup(3)
        screens.append(str(client.screenshot()))
        checks.append('An empty installation starts the native wizard and saves profile info; guest setup needs no online service')
        # Resume from an interrupted page using that same test-owned user data.
        data = client.folder / 'appdata'; client.stop()
        client = Client(args.exe.resolve(), args.iso.resolve(), data, output / 'resume', 'Startup review',
            window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        startup(3)
        assert (client.data / 'content/hub-profile.txt').read_text().splitlines()[:2] == ['Startup review', 'A new player.']
        checks.append('Restart resumes controller setup and keeps the saved profile info')
        client.click('setup-skip-controller'); startup(8)
        client.click('setup-back'); startup(3)
        client.click('setup-skip-controller'); startup(8)
        client.element('create-clone'); screens.append(str(client.screenshot()))
        client.enter('clone-name', 'First Mario'); client.click('create-clone')
        startup(9, timeout=120); screens.append(str(client.screenshot()))
        assert len(list((client.data / 'content/installed').glob('*.fmi'))) == 1
        client.click('setup-next'); client.element('play-lobby')
        screens.append(str(client.screenshot()))
        checks.append('Cloning installs one independent fighter and finishes on character select')
        data = client.folder / 'appdata'; client.stop()
        client = Client(args.exe.resolve(), args.iso.resolve(), data, output / 'complete', 'Startup review',
            window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        client.click('nav-6'); client.click('settings-startup'); startup(0)
        checks.append('Completed setup stays completed on restart and can be rerun from Settings')
        (output / 'receipt.json').write_text(json.dumps({
            'checks': checks, 'screenshots': screens,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['The UI preflight skips hardware calibration. Measured input/persistence and the real SDL provider have separate focused tests; the user will test the guided capture.'],
        }, indent=2) + '\n')
    finally:
        if client: client.stop()
    print(f'{len(checks)} startup checks passed; {len(screens)} screenshots')


if __name__ == '__main__':
    main()
