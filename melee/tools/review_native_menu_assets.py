"""Capture the retail menu scenes and the adapted hub in an isolated client.

Screenshots are visual references. This tool does not assert stylistic parity.
The game stays hidden and muted; no public messages or services are changed.
"""
import argparse
from pathlib import Path
import time

from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                    output / 'client', 'Menu Reference', window_size=(960, 540),
                    launch_args=['--retail-menu'])
    images = []

    def capture(name):
        images.append((name, str(client.screenshot())))

    def tap(button):
        client.tap(button)
        time.sleep(.6)

    try:
        # Allow the stock materials to appear before recording references.
        time.sleep(10)
        capture('main')
        tap('a')
        capture('single-player')
        tap('b')
        tap('down')
        tap('a')
        capture('versus')
        tap('b')
        tap('down')
        tap('down')
        tap('a')
        capture('options')
        tap('b')
        tap('up')
        tap('up')
        tap('a')
        tap('a')
        client.element('nav-1')
        time.sleep(5)
        client.click('nav-1')
        client.element('create-lobby')
        time.sleep(2)
        capture('adapted-lobbies')
        (output / 'assets.txt').write_text(client.inspect('hub-assets'))
        client.click('nav-0')
        client.element('quick-singles')
        capture('adapted-quick-play')
        assert client.process.poll() is None
    finally:
        (output / 'images.txt').write_text('\n'.join(name + ': ' + path for name, path in images))
        client.stop()
    print('Retail menu references and adapted hub screenshots captured.', flush=True)


if __name__ == '__main__':
    main()
