"""Review CSS-owned lobby controls, cards, chat and fighter sharing in two hidden, muted clients."""
from argparse import ArgumentParser
import json
import os
from pathlib import Path
import re
import secrets
import time

from review_native_css_room import Client
from review_native_release import close_window
from review_native_room_rotation import click_card


def wait(client, element, pattern, timeout=25):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        text = client.inspect(element)
        if re.search(pattern, text):
            return text
        time.sleep(.15)
    raise AssertionError(text)


def main():
    parser = ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'profile', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True, help='Isolated local signaling service')
    args = parser.parse_args()
    if not (args.profile / 'Freak Melee/css-layout.bin').is_file():
        parser.error('Use a populated test profile with an installed custom fighter on its CSS.')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    os.environ['FREAK_MELEE_CHAT_ENDPOINT'] = 'http://127.0.0.1:1'
    os.environ['FREAK_MELEE_COMMUNITY_ENDPOINT'] = 'http://127.0.0.1:1'
    os.environ['FREAK_MELEE_MENU_MOTION'] = '0'
    room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
    clients, shots = [], {}
    result = {'status': 'RUNNING', 'room': room, 'shots': shots}

    def shot(client, name):
        shots[name] = str(client.screenshot())

    try:
        for i in range(2):
            c = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output / str(i),
                       f'CSS feedback {i + 1}', window_size=(1440, 900))
            clients.append(c)
            c.click('nav-1')
            c.enter('lobby-code', room)
            c.click('join-lobby')
            wait(c, 'css-cards', r'online=1 count=4')
        a, b = clients
        for index, c in enumerate(clients):
            for _ in range(100):
                cards = c.inspect('css-cards')
                if len(re.findall(r'canonical=[0-3] ', cards)) == 2:
                    break
                time.sleep(.1)
            else:
                raise AssertionError(cards)
            assert re.search(rf'display={index} .*local=1 .* card={index}\b', cards), cards
            state, text = c.state()
            tx, ty = map(float, re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+)', text).groups())
            c.move(tx - 2.7, ty + 2)
            if c.state()[0]['hand'] != 1:
                c.tap('b')
            c.tap('a')
            assert c.state()[0]['chosen'] == 1
        shot(a, 'cards')
        a.enter('css-chat-input', 'Hello from the CSS. é')
        assert 'focused=1' in a.inspect('css-chat')
        a.command('ui-key', 'enter')
        wait(b, 'lobby-chat-state', r'Hello from the CSS\. é')
        b.enter('css-chat-input', 'A longer message that should wrap within the compact chat without reaching the cards.')
        b.command('ui-key', 'enter')
        wait(a, 'lobby-chat-state', r'A longer message')
        a.command('ui-key', 'escape')
        b.command('ui-key', 'escape')
        shot(a, 'chat')
        a.resize(960, 540)
        time.sleep(.3)
        shot(a, 'chat-narrow')
        a.resize(1440, 900)
        a.click('play-rules')
        a.click('rules-next-page')
        a.click('rules-next-page')
        a.element('change-listing')
        a.element('rotation-policy')
        shot(a, 'css-rules')
        a.tap('start')
        assert '[hub-inspect] match-rules' in a.inspect('match-rules'), 'START escaped the CSS rules view'
        a.element('change-listing')
        click_card(b, 1, 0)
        for _ in range(100):
            if len(re.findall(r'canonical=[0-3] ', a.inspect('css-cards'))) == 1:
                break
            time.sleep(.1)
        else:
            raise AssertionError('Releasing the card did not update the CSS cards')
        click_card(b, 1, 0, mouse=True)
        for _ in range(100):
            if len(re.findall(r'canonical=[0-3] ', a.inspect('css-cards'))) == 2:
                break
            time.sleep(.1)
        else:
            raise AssertionError('Player could not reclaim their open card')
        b.element('play-ready')
        initial = a.inspect('lobby-listing')
        expected = 0 if 'listed=1' in initial else 1
        a.command('ui-move', *a.element('change-listing'))
        a.tap('a', frames=40)
        wait(a, 'lobby-listing', f'listed={expected}')
        assert 'absent' in a.inspect('play-workshop')
        a.click('rules-close')
        a.element('play-ready')
        # Move the player's cookie off the source tile before picking up the
        # tile itself; cookies intentionally take priority over tile sharing.
        cookie, _ = a.state()
        a.command('ui-drag', *a.pixel(cookie['cx'] - 2.7, cookie['cy'] + 2), *a.pixel(0, 5))
        time.sleep(.2)
        assert a.state()[0]['holding'] == 0
        # Drag a custom tile onto the remote card's new, compact hit area.
        text = a.inspect('css-cards')
        remote = re.search(r'\[css-card\].*canonical=[01] local=0 .*center=([\d.-]+),([\d.-]+)', text)
        assert remote, text
        state, text = a.state()
        tx, ty = map(float, re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+)', text).groups())
        a.command('ui-drag', *a.pixel(tx, ty), *a.pixel(*map(float, remote.groups())))
        a.element('confirm-gift')
        shot(a, 'fighter-recipient')
        a.command('ui-key', 'escape')
        a.click('play-lobby')
        a.element('create-lobby')
        assert 'absent' in a.inspect('lobby-ready')
        a.click('return-session')
        a.element('play-ready')
        result.update(status='PASS', scope='Four authored cards with local-first identities, selected fighters, CSS chat including UTF-8/wrapping, stock header/Back hotspots, CSS-owned rules, modal START/B consumption, player card claims and releases, held A and compact remote-card fighter delivery selection.')
    except Exception as error:
        result.update(status='FAIL', error=str(error))
        for i, c in enumerate(clients):
            if c.process.poll() is None:
                shot(c, f'failure-{i}')
                (output / f'failure-{i}.txt').write_text(c.inspect('css-selection'), encoding='utf-8')
        raise
    finally:
        for c in clients:
            if c.process.poll() is None:
                close_window(c.process.pid)
                c.process.wait(timeout=20)
        (output / 'review.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps(result))


if __name__ == '__main__':
    main()
