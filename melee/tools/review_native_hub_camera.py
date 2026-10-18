"""Review shared stock camera orbit, pointer projection and native hub clicks.

Uses one hidden muted client with an isolated profile. No public chat is sent.
"""
import argparse
import math
from pathlib import Path
import re
import time

from review_native_css_room import Client


def pointer(client):
    text = client.inspect('hub-pointer')
    match = re.search(r'local=([\d.-]+),([\d.-]+) screen=([\d.-]+),([\d.-]+) mouse=1', text)
    assert match, text
    return tuple(map(float, match.groups()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                    output / 'client', 'Camera Review', window_size=(960, 540))
    evidence = []
    try:
        client.element('nav-1')
        client.screenshot()
        for window in [(960, 540), (1280, 720), (800, 800)]:
            client.resize(*window)
            time.sleep(.3)
            client.screenshot()  # Obtain the actual inner dimensions.
            px, py = client.canvas(416, 312)
            width, height = client.size
            scale = min(width / 640, height / 480)
            expected = ((px - (width - 640 * scale) * .5) / (640 * scale),
                        (py - (height - 480 * scale) * .5) / (480 * scale))
            neutral = None
            for cx, cy in [(0, 0), (80, 0), (-80, 0), (0, 80), (0, -80), (65, 50), (-65, -50)]:
                client.command('ui-move', px, py)
                client.command('hold', 0, 'sx=0', 'sy=0', f'cx={cx}', f'cy={cy}')
                time.sleep(.2)
                current = pointer(client)
                assert abs(current[2] - expected[0]) < .002 and abs(current[3] - expected[1]) < .002, (window, cx, cy, current, expected)
                camera = client.inspect('hub-camera')
                eye = tuple(map(float, re.search(r'camera-eye=([\d.-]+),([\d.-]+),([\d.-]+)', camera).groups()))
                if not cx and not cy:
                    neutral = current[:2]
                    assert abs(eye[0]) < .001 and abs(eye[1]) < .001 and abs(eye[2]-51) < .001, eye
                else:
                    assert math.hypot(eye[0], eye[1]) > 1, eye
                    assert math.dist(current[:2], neutral) > .5, current
                # A stationary mouse must stay under the same screen pixel
                # while the shared camera is updated by later simulation ticks.
                time.sleep(.15)
                fixed = pointer(client)
                assert math.dist(fixed[2:], expected) < .003, (fixed, expected)
                client.click('nav-1')
                client.element('create-lobby')
                client.click('nav-5')
                client.element('local-practice')
                evidence.append(f'{window} stick={cx},{cy} eye={eye} pointer={current}')
                if window == (960, 540) and (cx, cy) in [(80, 0), (65, 50)]:
                    client.screenshot()
            client.command('hold', 0, 'sx=0', 'sy=0', 'cx=0', 'cy=0')
        time.sleep(.2)
        client.click('your-profile')
        client.element('profile-bio')
        client.enter('profile-bio', 'Projected camera review.')
        client.click('save-bio')
        client.command('ui-key', 'escape')
        client.element('local-practice')
        assert client.process.poll() is None
        (output / 'passed.txt').write_text('PASS: seven camera poses at three window sizes; projected clicks, fixed mouse position, profile editing and return.\n')
        print((output / 'passed.txt').read_text().strip(), flush=True)
    finally:
        (output / 'projection.txt').write_text('\n'.join(evidence))
        client.stop()


if __name__ == '__main__':
    main()
