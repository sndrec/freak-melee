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
        client.element('account-login-mode'); screens.append(str(client.screenshot()))
        client.click('setup-next'); startup(2)
        client.enter('setup-name', 'Startup review'); client.enter('setup-bio', 'A new player.')
        screens.append(str(client.screenshot()))
        client.click('setup-next'); startup(3)
        screens.append(str(client.screenshot()))
        checks.append('Account choices precede optional guest profile setup; an offline guest saves profile info without an online service')
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

        client.enter('setup-tile-name', 'MARIO!'); screens.append(str(client.screenshot()))
        client.click('setup-portrait-save')
        startup(11, timeout=120); screens.append(str(client.screenshot()))
        from PIL import Image
        image_path = client.data / 'content/startup-portrait.png'
        with Image.open(image_path) as image:
            assert image.mode == 'RGBA' and image.size == (464, 304)
            assert image.getchannel('A').getextrema() == (0, 255)
        portraits = list((client.data / 'projects').rglob('portrait.png'))
        assert any(p.read_bytes() == image_path.read_bytes() for p in portraits)
        client.element('setup-place-area'); screens.append(str(client.screenshot()))
        assert len(list((client.data / 'content/installed').glob('*.fmi'))) == 1
        layout_path = client.data / 'css-layout.bin'
        import struct
        def saved_count():
            return struct.unpack_from('<H', layout_path.read_bytes(), 4)[0] if layout_path.exists() else 0
        assert saved_count() == 0, 'A placement preview must not save the tile'
        before_cancel = layout_path.read_bytes() if layout_path.exists() else None
        client.click('setup-back'); startup(10)
        assert (layout_path.read_bytes() if layout_path.exists() else None) == before_cancel, 'Cancelling placement must preserve the original layout'
        client.click('setup-portrait-save'); startup(11)
        area = client.inspect('setup-place-area')
        m = re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', area)
        ax, ay, aw, ah = map(float, m.groups())
        target = round(ax + .72 * aw), round(ay + .38 * ah)
        client.command('ui-move', *target); time.sleep(.2)
        preview = client.inspect('startup-stamp')
        xy = tuple(map(int, re.search(r'xy=(\d+),(\d+)', preview).groups()))
        client.command('tap', 0, 'sx=40', 'frames=8'); time.sleep(.3)
        moved = tuple(map(int, re.search(r'xy=(\d+),(\d+)', client.inspect('startup-stamp')).groups()))
        assert moved[0] > xy[0], (xy, moved)
        client.command('ui-move', *target); time.sleep(.2)
        xy = tuple(map(int, re.search(r'xy=(\d+),(\d+)', client.inspect('startup-stamp')).groups()))
        client.command('ui-click', *target); time.sleep(.3)
        client.element('play-lobby')
        assert (client.data / 'content/startup.txt').read_text().splitlines()[0].split()[2] == '1'
        layout = layout_path.read_bytes()
        assert struct.unpack_from('<H', layout, 4)[0] == 1
        assert struct.unpack_from('<HH', layout, 46) == xy
        manifests = [p.read_text() for p in (client.data / 'projects').glob('*/project.txt')]
        assert len(manifests) == 1 and manifests[0].startswith('title=Mario\n') and 'tile_name=MARIO!\n' in manifests[0]
        checks.append('The short tile name stays separate from the fighter title; mouse and stick move the actual CSS preview, Back cancels, and dropping saves one tile at the chosen coordinates')
        screens.append(str(client.screenshot()))
        checks.append('Cloning installs one independent fighter and finishes on character select')
        data = client.folder / 'appdata'; client.stop()
        client = Client(args.exe.resolve(), args.iso.resolve(), data, output / 'complete', 'Startup review',
            window_size=(1280, 960), chat_endpoint='http://localhost:1', write_profile=False)
        client.click('nav-5'); client.click('portrait-fighter')
        client.element('portrait-mode-card'); screens.append(str(client.screenshot()))
        project = next((client.data / 'projects').iterdir())
        tile_before = (project / 'portrait.png').read_bytes()
        resources_before = (project / 'resources.fmr').read_bytes()
        client.click('portrait-mode-card'); screens.append(str(client.screenshot()))
        client.click('setup-portrait-save'); client.element('portrait-fighter')
        assert (project / 'portrait.png').read_bytes() == tile_before
        assert (project / 'resources.fmr').read_bytes() != resources_before
        with Image.open(project / 'player-card.png') as card:
            assert card.size == (136, 188)
        checks.append('Workshop portrait tools load the owned fighter model and save a separate player-card image while preserving its CSS tile')
        client.click('portrait-fighter'); client.element('setup-tile-name')
        client.enter('setup-tile-name', 'A ridiculously long Mario fighter')
        resources_after = (project / 'resources.fmr').read_bytes()
        client.click('setup-portrait-save'); client.element('portrait-fighter')
        assert (project / 'resources.fmr').read_bytes() == resources_after
        with Image.open(project / 'portrait.png') as picture:
            assert picture.size == (464, 248)
        checks.append('Editing the tile name changes the capture aspect ratio and preserves the independently edited player-card portrait')
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
