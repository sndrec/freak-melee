"""Check native cookie placement with an isolated copy of a test profile.

Supply a profile produced by review_native_hub.py. The test drives the actual
controller path, records CSS state and screenshots, then stops its own client.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--profile', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    exe, iso = args.exe.resolve(), args.iso.resolve()
    output = args.output.resolve()
    output.mkdir(exist_ok=False, parents=True)
    profile = output / 'appdata'
    shutil.copytree(args.profile.resolve(), profile)
    control = exe.with_name('melee_native_control.exe')
    identity = f'css-review-{os.getpid()}'
    env = dict(os.environ, LOCALAPPDATA=str(profile),
               FREAK_MELEE_CONTROL_ID=identity,
               FREAK_MELEE_CHAT_ENDPOINT='http://127.0.0.1:1')
    stderr = output / 'stderr.log'

    def command(*parts):
        subprocess.run([str(control), '--id', identity, *map(str, parts)],
                       check=True, capture_output=True,
                       creationflags=subprocess.CREATE_NO_WINDOW)

    def inspect(element):
        start = stderr.stat().st_size
        command('ui-inspect', element)
        for _ in range(30):
            time.sleep(.1)
            with stderr.open('r', errors='replace') as file:
                file.seek(start)
                text = file.read()
            if re.search(r'\[(?:css-selection|hub-inspect|ui-inspect|play-menu)\]', text):
                return text
            assert client.poll() is None, f'Native client exited {client.returncode}'
        raise RuntimeError(f'Inspection stalled: {element}')

    def click(element):
        text = inspect(element)
        match = re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', text)
        assert match, text
        x, y, w, h = map(float, match.groups())
        command('ui-click', round(x+w/2), round(y+h/2))
        time.sleep(.35)

    def tap(*parts, frames=1):
        command('tap', 0, *parts, f'frames={frames}')
        time.sleep(frames / 60 + .1)

    def screenshot():
        before = len(list((output / 'screenshots').glob('*.png')))
        command('screenshot')
        for _ in range(150):
            assert client.poll() is None
            if len(list((output / 'screenshots').glob('*.png'))) > before:
                return
            time.sleep(.1)
        raise RuntimeError('Screenshot was not rendered')

    def state():
        text = inspect('css-selection')
        rows = re.findall(r'\[css-slot\] slot=(\d+) kind=(\d+) chosen=(\d+) '
                          r'hand=(\d+) holding=(\d+) hand_pos=([\d.-]+),([\d.-]+) '
                          r'cookie_pos=([\d.-]+),([\d.-]+) character=(\d+) custom=(\d+)', text)
        if not rows:
            return None, text
        slots = []
        for row in rows:
            slots.append(dict(zip(['slot', 'kind', 'chosen', 'hand', 'holding',
                                  'x', 'y', 'cx', 'cy', 'character', 'custom'],
                                 map(float, row))))
        return slots[0], text

    def move(x, y):
        for axis, target in [('x', x), ('y', y)]:
            for _ in range(40):
                current, _ = state()
                assert current is not None
                error = target - current[axis]
                if abs(error) < .25:
                    break
                magnitude = 80 if abs(error) > 3 else 40 if abs(error) > .8 else 20
                speed = (magnitude * magnitude - 200) * .0002
                frames = min(20, max(1, round(abs(error) / speed)))
                tap(f's{axis}={magnitude if error > 0 else -magnitude}', frames=frames)
            else:
                raise RuntimeError(f'Could not move hand to {axis}={target}')

    with (output / 'stdout.log').open('w') as out, stderr.open('w') as err:
        client = subprocess.Popen([str(exe), '--iso', str(iso), '--hidden', '--mute'],
                                  cwd=output, env=env, stdout=out, stderr=err,
                                  creationflags=subprocess.CREATE_NO_WINDOW)
        (output / 'pid.txt').write_text(str(client.pid))
        try:
            for _ in range(180):
                assert client.poll() is None
                try:
                    text = inspect('local-practice')
                except subprocess.CalledProcessError:
                    time.sleep(.25)
                    continue
                match = re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+)', text)
                if match:
                    x, y, w, h = map(float, match.groups())
                    command('ui-click', round(x + w / 2), round(y + h / 2))
                    break
                time.sleep(.25)
            else:
                raise RuntimeError('The home hub did not become available')
            time.sleep(.5)
            initial, text = state()
            assert initial is not None
            tile = re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+) custom=1', text)
            assert tile, 'This fixture needs an installed custom fighter'
            tx, ty = map(float, tile.groups())
            move(tx - 2.7, ty + 2)
            current, _ = state()
            if current['hand'] != 1:
                tap('b')
            tap('a')
            selected, text = state()
            assert selected['custom'] == selected['chosen'] == 1 and selected['holding'] == 0, text
            tap('b')
            move(-2.7, -8)
            tap('a')
            placed, text = state()
            assert placed['custom'] == placed['chosen'] == 1 and placed['holding'] == 0, text
            assert abs(placed['cx']) < .4 and abs(placed['cy'] + 10) < .4, text
            time.sleep(1)
            later, text = state()
            assert later['cx'] == placed['cx'] and later['cy'] == placed['cy'], text
            screenshot()
            tap('b')
            held, text = state()
            assert held['holding'] == 1 and 'ready=0' in text, text
            tap('start')
            assert state()[0] is not None, 'START accepted a carried cookie'
            tap('a')
            time.sleep(.3)
            final, text = state()
            assert final['custom'] == 1 and 'ready=1' in text, text
            # An empty stock roster must still support opening a CPU slot.
            # Retail's random-selection loop only knows stock portraits.
            move(-16, -2.2)
            tap('a')
            _, text = state()
            assert re.search(r'\[css-slot\] slot=1 kind=1 chosen=1 .*custom=1', text), text
            screenshot()
            # The detailed collection owns mouse input while its panel covers
            # the game, including the close button inside the CSS viewport.
            click('play-workshop')
            click('nav-6')
            click('settings-costumes')
            click('costume-close')
            click('settings-rules')
            click('rules-close')
            click('settings-packages')
            click('collection-close')
            click('hub-close')
            assert 'absent' in inspect('right-rail')
            click('play-ready')
            for _ in range(180):
                assert client.poll() is None
                if '[css-selection] practice' in inspect('css-selection'):
                    break
                time.sleep(.25)
            else:
                raise RuntimeError('Confirmed selection did not enter practice')
            screenshot()
            # The practice strip opens the controller-navigable hub without
            # taking over ordinary match buttons. Return by the same native UI.
            click('play-lobby')
            tap('b')
            inspect('play-selection')
            # Retail's Ready/Go countdown rejects pause input. The screenshot
            # above records that intro; allow it to finish before opening pause.
            time.sleep(4)
            tap('start')
            time.sleep(.4)
            tap('z')
            click('hub-close')
            click('play-selection')
            for _ in range(180):
                if 'absent' not in inspect('nav-5'):
                    break
                time.sleep(.15)
            else:
                raise RuntimeError('Native practice return did not reach home CSS')
            click('nav-5')
            click('local-practice')
            assert state()[0] is not None
            screenshot()
            (output / 'result.txt').write_text(
                'PASS: custom cookie selection, empty-space placement, stable position, '
                'pick-up, ready guard, custom CPU, native ready/practice/return controls, paused controller hub, '
                'costumes/rules/packages tool return, and removed room rails.\n')
            print((output / 'result.txt').read_text().strip())
        finally:
            if client.poll() is None:
                client.terminate()
                client.wait(timeout=15)


if __name__ == '__main__':
    main()
