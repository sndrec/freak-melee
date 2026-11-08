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
        client.click('create-clone')
        startup(9); screens.append(str(client.screenshot()))
        assert not list((client.data / 'content/installed').glob('*.fmi'))
        client.click('setup-confirm-fighter'); startup(10)
        client.element('setup-portrait-save'); screens.append(str(client.screenshot()))

        def camera():
            text = client.inspect('startup-camera')
            m = re.search(r'yaw=([\d.-]+) pitch=([\d.-]+) zoom=([\d.-]+) pan=([\d.-]+),([\d.-]+)', text)
            if not m: raise RuntimeError(text)
            return tuple(map(float, m.groups()))

        initial = camera()
        client.command('tap', 0, 'cx=80', 'cy=40', 'frames=12'); time.sleep(.4)
        orbited = camera()
        assert orbited[0] < initial[0] - .1 and orbited[1] > initial[1] + .05, (initial, orbited)
        client.command('tap', 0, 'right', 'up', 'frames=10'); time.sleep(.35)
        panned = camera()
        assert panned[3] > .05 and panned[4] > .05, panned
        client.command('tap', 0, 'y', 'frames=8'); time.sleep(.3)
        assert camera()[2] > initial[2] + .05
        client.click('setup-portrait-reset')
        assert camera() == initial

        # Hold A over the picture: the main stick pans it without moving the hand.
        element = client.inspect('setup-portrait-reset')
        m = re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', element)
        bx, by, bw, bh = map(float, m.groups())
        pixel_scale = bw / 150
        client.command('ui-move', round(bx + (203 - 363) * pixel_scale), round(by + (265 - 368) * pixel_scale))
        hand_before = client.inspect('hub-pointer')
        client.command('tap', 0, 'a', 'sx=64', 'sy=32', 'frames=10'); time.sleep(.35)
        hand_after = client.inspect('hub-pointer')
        assert camera()[3] > .03 and camera()[4] > .01
        point = lambda s: re.search(r'local=([\d.-]+),([\d.-]+)', s).groups()
        assert point(hand_before) == point(hand_after), (hand_before, hand_after)
        client.click('setup-portrait-reset')
        checks.append('Gallery C-stick orbit, D-pad and A+stick pan, X/Y zoom and reset adjust the frozen portrait; panning consumes hand motion')

        client.click('setup-portrait-save')
        startup(11, timeout=120); screens.append(str(client.screenshot()))
        assert len(list((client.data / 'content/installed').glob('*.fmi'))) == 1
        from PIL import Image
        image_path = client.data / 'content/startup-portrait.png'
        with Image.open(image_path) as image:
            assert image.mode == 'RGBA' and image.size == (256, 256)
            assert image.getchannel('A').getextrema() == (0, 255)
        portraits = list((client.data / 'projects').rglob('portrait.png'))
        assert any(p.read_bytes() == image_path.read_bytes() for p in portraits)
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
