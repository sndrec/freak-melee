"""Review authored native menu transitions and scene reconstruction.

One hidden, muted client uses an isolated copy of a fixture profile. It captures
forward/backward motion, checks that moving controls cannot receive clicks, and
reopens the hub after entering and exiting practice. No public chat is sent.
"""
import argparse
import os
from pathlib import Path
import re
import time

from review_native_css_room import Client
from review_native_quick_play import choose


def capture_motion(client, target, output, label, backwards=False, animated=True):
    ignored = client.element('nav-2')
    if backwards:
        client.command('ui-key', 'escape')
    else:
        client.command('ui-click', *client.element(target))
    text = client.inspect('hub-motion')
    assert ('active=1' if animated else 'active=0') in text, text
    if animated:
        assert re.search(r'frame=(?:[0-9]|1[0-9])\.', text), text
        client.command('ui-click', *ignored)  # A stale button hit must be ignored.
    image = client.screenshot()
    (output/(label+'.txt')).write_text(text+'\n'+str(image)+'\n')
    target_page = re.search(r'to=(\d+)', text)[1]
    deadline = time.monotonic()+3
    while time.monotonic() < deadline:
        ended = client.inspect('hub-motion')
        if 'active=0' in ended:
            assert f'to={target_page} ' in ended, ended
            break
    else:
        raise RuntimeError('Menu transition did not finish')
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--reduced-motion', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    os.environ['FREAK_MELEE_MENU_MOTION'] = '0' if args.reduced_motion else '1'
    animated = not args.reduced_motion
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                    output/'client', 'Motion Review', window_size=(960,540))
    try:
        client.element('nav-1'); client.screenshot()
        capture_motion(client, 'nav-1', output, 'forward', animated=animated)
        client.element('create-lobby'); client.screenshot()
        capture_motion(client, '', output, 'backward', backwards=True, animated=animated)
        client.element('local-practice'); client.screenshot()
        # Profiles use a separate page and return to the previous page.
        capture_motion(client, 'your-profile', output, 'profile', animated=animated)
        client.element('profile-bio')
        client.enter('profile-bio', 'Animation review.')
        client.click('save-bio')
        capture_motion(client, '', output, 'profile-back', backwards=True, animated=animated)
        client.element('local-practice')
        client.click('local-practice'); choose(client)
        client.click('play-ready'); client.element('play-selection')
        time.sleep(3)
        client.tap('start'); client.tap('z')
        client.element('nav-6')
        capture_motion(client, 'nav-6', output, 'practice', animated=animated)
        client.element('controls'); client.screenshot()
        client.click('hub-close')
        client.command('tap', 0, 'l', 'r', 'a', 'start', 'frames=2')
        deadline = time.monotonic()+30
        while time.monotonic() < deadline:
            if '[css-slot]' in client.inspect('css-selection'):
                break
            time.sleep(.1)
        else:
            raise RuntimeError('Practice did not return to CSS')
        client.element('nav-1')
        capture_motion(client, 'nav-1', output, 'reentered-css', animated=animated)
        client.element('create-lobby'); client.screenshot()
        stdout = (client.folder/'stdout.log').read_text(errors='replace')
        assert '[netplay] FAILED:' not in stdout
        assert client.process.poll() is None
        (output/'passed.txt').write_text('Native forward/back/profile transitions and CSS/practice scene reconstruction passed.\n')
        print((output/'passed.txt').read_text().strip(), flush=True)
    finally:
        client.stop()


if __name__ == '__main__':
    main()
