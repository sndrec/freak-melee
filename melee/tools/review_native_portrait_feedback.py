"""Review animation/frame selection and saved portraits in one isolated, hidden, muted client."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import time
from PIL import Image
from review_native_css_room import Client
from review_native_release import close_window


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'profile', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    for service in ('SERVICE', 'CHAT', 'COMMUNITY'):
        os.environ['FREAK_MELEE_' + service + '_ENDPOINT'] = 'http://127.0.0.1:1'
    os.environ['FREAK_MELEE_MENU_MOTION'] = '0'
    client = None
    result = {'status': 'RUNNING', 'screenshots': {}}

    def shot(name):
        result['screenshots'][name] = str(client.screenshot())

    def camera():
        text = client.inspect('startup-camera')
        assert re.search(r'active=1 subjects=[12] ', text), text
        return text

    def choose(name):
        listing = client.inspect('portrait-animations')
        names = dict((n, int(i)) for i, n in re.findall(r'portrait-animation index=(\d+) name=(\w+)', listing))
        assert name in names, listing
        index = names[name]
        client.click('portrait-animation')
        for _ in range(index // 8):
            client.click('portrait-animation-next')
        client.click('portrait-animation-' + str(index))
        assert 'animation=' + name + ' ' in camera()

    def frame(value):
        client.enter('portrait-frame', str(value))
        client.command('ui-key', 'enter')
        time.sleep(.2)
        text = camera()
        assert 'frame=' + format(float(value), '.6f') in text, text

    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output / 'client',
                        'Portrait feedback', window_size=(1440, 900))
        client.click('nav-5')
        client.click('clone-fighter')
        client.enter('clone-name', 'Portrait Feedback Mario')
        client.click('create-clone')
        client.element('edit-fighter')
        client.click('portrait-fighter')
        client.element('portrait-mode-card')
        initial = camera()
        draft = Path(re.search(r'portrait-draft path=([^\r\n]+)', initial)[1])
        original_tile = (draft / 'portrait.png').read_bytes()
        shot('initial-tile')
        choose('AttackAirF')
        frame(17)
        shot('aerial-tile')
        client.click('portrait-mode-card')
        assert 'animation=Wait1 ' in camera()
        choose('AttackHi3')
        frame(8)
        shot('posed-card')
        client.command('ui-move', *client.element('portrait-frame-next'))
        client.tap('a', frames=30)
        assert 'frame=9.000000' in camera(), 'Held A repeated a frame-step button'
        client.click('portrait-mode-tile')
        assert 'animation=AttackAirF ' in camera() and 'frame=17.000000' in camera()
        client.click('portrait-mode-card')
        assert 'animation=AttackHi3 ' in camera() and 'frame=9.000000' in camera()
        client.click('setup-portrait-save')
        client.element('portrait-fighter')
        card_path = draft / 'player-card.png'
        first_card = card_path.read_bytes()
        assert (draft / 'portrait.png').read_bytes() == original_tile
        with Image.open(card_path) as image:
            assert image.size == (136, 188) and image.mode == 'RGBA'
            alpha = image.getchannel('A')
            assert alpha.getextrema() == (0, 255) and alpha.getbbox()
            shadow = [(r, g, b) for r, g, b, a in image.getdata() if 107 <= a <= 111]
            assert len(shadow) > 100, 'Stock-style translucent shadow is absent'
            assert sum(max(rgb) > 3 for rgb in shadow) < len(shadow) * .02, 'Shadow kept the fighter texture colors'
        client.click('portrait-fighter')
        client.click('portrait-mode-card')
        restored = camera()
        assert 'animation=AttackHi3 ' in restored and 'frame=9.000000' in restored, restored
        client.enter('portrait-frame', '20')
        client.click('setup-portrait-save')
        client.element('portrait-fighter')
        assert card_path.read_bytes() != first_card, 'Changing the pose did not affect the PNG'
        client.click('portrait-fighter')
        choose('AttackAirF')
        frame(17)
        shot('tile-capture')
        client.click('setup-portrait-save')
        client.element('portrait-fighter')
        assert (draft / 'portrait.png').read_bytes() != original_tile
        assert card_path.read_bytes() != first_card
        client.click('portrait-fighter')
        restored = camera()
        assert 'animation=AttackAirF ' in restored and 'frame=17.000000' in restored, restored
        client.click('portrait-mode-card')
        assert 'animation=AttackHi3 ' in camera() and 'frame=20.000000' in camera()
        client.resize(960, 540)
        shot('card-narrow')
        client.resize(1440, 900)
        client.click('portrait-back')
        client.click('local-practice')
        time.sleep(.5)
        layout = (client.data / 'css-layout.bin').read_bytes()
        identity = (draft / 'installed.id').read_text().strip()
        index = next(i for i in range(struct.unpack_from('<H', layout, 4)[0]) if layout[14 + i*48:46 + i*48].hex() == identity)
        text = client.inspect('css-selection')
        tile = re.search(r'\[css-tile\] index=' + str(index) + r' center=([\d.-]+),([\d.-]+)', text)
        assert tile, text
        tx, ty = map(float, tile.groups())
        client.move(tx - 2.7, ty + 2)
        if client.state()[0]['hand'] != 1:
            client.tap('b')
        client.tap('a')
        assert client.state()[0]['chosen'] == 1
        shot('runtime-css')
        bounds = client.inspect('css-cards')
        original_rect = re.search(r'stock-portrait x=([\d.-]+) y=([\d.-]+) width=([\d.-]+) height=([\d.-]+)', bounds)
        assert original_rect, bounds
        px, py, width, height = map(float, original_rect.groups())
        assert abs(width - 14.199951) < .001 and abs(height - 18.999756) < .001, bounds
        result['original_portrait_rect'] = [px, py, width, height]
        client.click('play-lobby')
        client.click('nav-5')
        client.click('clone-fighter')
        # Ice Climbers is the ninth selectable template in the native grid.
        client.command('ui-click', *client.canvas(524, 184))
        client.enter('clone-name', 'Portrait Feedback Climbers')
        client.click('create-clone')
        client.element('edit-fighter')
        client.click('portrait-fighter')
        client.element('portrait-mode-card')
        assert 'subjects=2' in camera()
        choose('AttackAirF')
        frame(10)
        client.click('portrait-mode-card')
        choose('AttackAirF')
        frame(10)
        shot('paired-card')
        client.click('setup-portrait-save')
        client.element('portrait-fighter')
        result.update(status='PASS', draft=str(draft), card_sha256=hashlib.sha256(card_path.read_bytes()).hexdigest(),
                      scope='Original P1 card preview; selectable fighter animations and exact frames; held-A edge behavior; separate mode poses, saved-camera reload and changed PNGs; RGBA output; native CSS placement; both window sizes; paired-fighter capture.')
    except Exception as error:
        result.update(status='FAIL', error=str(error))
        if client and client.process.poll() is None:
            shot('failure')
        raise
    finally:
        if client and client.process.poll() is None:
            close_window(client.process.pid)
            client.process.wait(timeout=20)
        (output / 'review.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result))


if __name__ == '__main__':
    main()

