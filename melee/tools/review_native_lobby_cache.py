"""Check selection cancellation and automatic cache while spectating in practice.

Uses hidden, muted native clients, isolated stores and a fresh private lobby.
"""
import argparse
from pathlib import Path
import re
import secrets
import time
from review_native_css_room import Client


def wait(client, predicate, label, timeout=40):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        text = client.inspect('css-cards')
        if predicate(text):
            return text
        time.sleep(.15)
    raise AssertionError(f'{label}: {text}')


def choose(client, tile):
    state, text = client.state()
    tx, ty = map(float, re.search(rf'\[css-tile\] index={tile} center=([\d.-]+),([\d.-]+)', text).groups())
    if state['hand'] != 1:
        client.tap('b')
    client.move(tx-2.7, ty+2)
    client.tap('a')
    assert client.state()[0]['chosen'] == 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(exist_ok=False, parents=True)
    room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
    (output / 'room.txt').write_text(room)
    clients = []
    try:
        for index in range(2):
            client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output / str(index),
                            f'Cache Review {index+1}', missing_first=index == 1)
            clients.append(client)
            client.click('nav-1')
            client.enter('lobby-code', room)
            client.click('join-lobby')
            client.wait_cards(index+1)
            client.click('lobby-ready')
        sender, receiver = clients
        cache = receiver.data / f'content/cache/{receiver.missing}.fmc'
        installed = receiver.data / f'content/installed/{receiver.missing}.fmi'
        roster = (receiver.data / 'css-layout.bin').read_bytes()
        choose(receiver, 0)
        choose(sender, 0)
        wait(receiver, lambda text: 'receiving=1' in text, 'download starts')
        choose(sender, 1)
        wait(receiver, lambda text: 'receiving=0' in text and 'portrait=1' in text, 'old download cancels')
        time.sleep(1)
        assert not cache.exists() and not installed.exists()
        receiver.screenshot()
        receiver.tap('b', 40)
        receiver.click('nav-1')
        for _ in range(2):
            receiver.command('ui-click', *receiver.canvas(406, 194.5))
            time.sleep(.6)
        receiver.wait_cards(1)
        receiver.click('lobby-ready')
        if receiver.state()[0]['holding']:
            receiver.tap('a')
        receiver.tap('start')
        for _ in range(200):
            if '[css-selection] practice' in receiver.inspect('css-selection'):
                break
            time.sleep(.1)
        else:
            raise AssertionError('Spectator did not enter practice')
        choose(sender, 0)
        wait(receiver, lambda text: cache.exists() and 'receiving=0' in text, 'spectator practice caches remote choice', timeout=60)
        assert not installed.exists() and (receiver.data / 'css-layout.bin').read_bytes() == roster
        time.sleep(4)
        receiver.screenshot()
        receiver.click('play-selection')
        receiver.click('nav-1')
        receiver.click('lobby-ready')
        wait(receiver, lambda text: re.search(r'\[css-card\].*local=0.*selected=1.*portrait=1', text), 'cached portrait on return')
        receiver.screenshot()
        (output / 'result.txt').write_text('PASS: changing selection cancels an incomplete automatic download; a spectator receives the next selected fighter during practice; roster remains unchanged; cached portrait appears on CSS return.\n')
        print((output / 'result.txt').read_text().strip())
    finally:
        for client in reversed(clients):
            client.stop()


if __name__ == '__main__':
    main()
