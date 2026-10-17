"""Review private lobby chat and paged room controls using hidden, muted clients.

Uses a fresh private room. Global chat is disabled; no public messages are sent.
"""
import argparse
from pathlib import Path
import re
import secrets
import struct
import time
from urllib.parse import urlencode
from websockets.sync.client import connect
from review_native_css_room import Client


def packet(kind, body=b''):
    return b'FMS1' + bytes([5, kind]) + struct.pack('>H', len(body)) + body


def wait_chat(client, count, text=None, timeout=30):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        state = client.inspect('lobby-chat-state')
        if f'ready=1 count={count} delivery=0' in state and (not text or text in state):
            return state
        time.sleep(.2)
    raise AssertionError(state)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', default='wss://freak-melee-signaling.sndrec32exe.workers.dev')
    parser.add_argument('--history-only', action='store_true')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(exist_ok=False, parents=True)
    clients, sockets = [], []
    room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
    (output / 'room.txt').write_text(room)
    try:
        for i in range(1 if args.history_only else 2):
            client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output / str(i), f'Lobby Review {i+1}')
            clients.append(client)
            client.click('nav-1')
            client.enter('lobby-code', room)
            client.click('join-lobby')
            client.wait_cards(i+1)
            client.click('lobby-chat')
            wait_chat(client, 0)
        if args.history_only:
            client = clients[0]
            for i in range(4):
                message = f'Long message {i+1}: ' + 'WIDE words ' * 43
                client.enter('lobby-message', message[:120])
                for offset in range(120, len(message), 120):
                    client.command('ui-text', message[offset:offset+120])
                client.command('ui-key', 'enter')
                wait_chat(client, i+1, message)
                time.sleep(.6)
            client.screenshot()
            assert 'enabled=1' in client.inspect('chat-older')
            client.command('ui-move', *client.element('chat-older'))
            client.tap('a')
            assert 'enabled=1' in client.inspect('chat-newer')
            client.screenshot()
            for _ in range(8):
                if 'enabled=0' in client.inspect('chat-newer'):
                    break
                client.click('chat-newer')
            else:
                raise AssertionError('History did not return to newest messages')
            (output / 'result.txt').write_text('PASS: long native lobby messages remain scrollable using the hand/controller Older button and Newer control.\n')
            print((output / 'result.txt').read_text().strip())
            return
        a, b = clients
        a.enter('lobby-message', 'Hello from the private lobby.')
        a.command('ui-key', 'enter')
        wait_chat(a, 1, 'Hello from the private lobby.')
        wait_chat(b, 1, 'Hello from the private lobby.')
        b.enter('lobby-message', 'This message stays in this room.')
        b.click('send-lobby-chat')
        wait_chat(a, 2, 'This message stays in this room.')
        wait_chat(b, 2)
        # Visiting global chat preserves the independent room conversation.
        a.click('nav-2')
        a.screenshot()
        a.click('nav-1')
        a.click('lobby-chat')
        wait_chat(a, 2)
        a.screenshot()
        # Add spectator-only sockets: chat requires no gameplay/ICE connection.
        for i in range(13):
            query = urlencode(dict(peer=f'review_spectator_{i:02}', name=f'Visitor {i:02}',
                players=0, active=0, waiting=0, build='freak-melee-native-dev-cache'))
            socket = connect(f'{args.endpoint}/v1/rooms/{room}/connect?{query}', max_queue=64, close_timeout=1)
            sockets.append(socket)
            assert socket.recv()[5] == 0x81
            assert socket.recv()[5] == 0x82
            time.sleep(.15)
        sockets[-1].send(packet(9))
        while sockets[-1].recv()[5] != 0x87:
            pass
        text = b'Spectator delivery without gameplay links.'
        sockets[-1].send(packet(8, struct.pack('>IH', 1, len(text)) + text))
        wait_chat(a, 3, text.decode())
        wait_chat(b, 3)
        a.click('people-next')
        a.screenshot()
        # All 15 members are reachable from the paged lobby, including last page.
        a.click('nav-1')
        a.click('lobby-next')
        a.click('lobby-next')
        a.screenshot()
        a.click('lobby-player')
        a.screenshot()
        # Return to the room and verify ordinary scene controls still work.
        a.command('ui-key', 'escape')
        a.click('lobby-ready')
        a.screenshot()
        assert 'online=1 count=2' in a.inspect('css-cards')
        state, text = a.state()
        tx, ty = map(float, re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+)', text).groups())
        a.move(tx-2.7, ty+2)
        if state['hand'] != 1:
            a.tap('b')
        a.tap('a')
        assert a.state()[0]['chosen'] == 1
        a.tap('start')
        for _ in range(200):
            if '[css-selection] practice' in a.inspect('css-selection'):
                break
            time.sleep(.1)
        else:
            raise AssertionError('Did not enter ready practice')
        time.sleep(4)
        a.tap('start')
        a.tap('z')
        a.click('nav-1')
        a.click('lobby-chat')
        b.enter('lobby-message', 'Chat still arrives during practice.')
        b.click('send-lobby-chat')
        wait_chat(a, 4, 'Chat still arrives during practice.')
        a.screenshot()
        (output / 'result.txt').write_text('PASS: native lobby chat, server acknowledgments, independent global view, spectator delivery, 15-member player paging, profile navigation, CSS return and chat while in ready practice.\n')
        print((output / 'result.txt').read_text().strip())
    finally:
        for socket in sockets:
            socket.close()
        for client in reversed(clients):
            client.stop()


if __name__ == '__main__':
    main()


