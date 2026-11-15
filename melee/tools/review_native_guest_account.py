"""Review guest-to-account lobby attachment in two hidden, muted clients."""
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
    for name in ['exe', 'iso', 'profile-a', 'profile-b', 'output']:
        parser.add_argument('--' + name, type=Path, required=True)
    for name in ['a-handle', 'b-handle', 'endpoint']:
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    clients, checks, screenshots = [], [], []

    def wait(client, tag, pattern, timeout=30):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            text = client.inspect(tag)
            if re.search(pattern, text): return text
            time.sleep(.2)
        raise RuntimeError(f'{tag} did not reach {pattern}: {text}\n{client.inspect("hub-account")}\n{client.inspect("room-state")}')

    def sign_in(client):
        client.click('nav-6')
        client.click('settings-category-3')
        client.click('settings-account')
        state = wait(client, 'hub-account', r'busy=0 signed-in=[01]')
        if 'signed-in=0' in state:
            client.click('account-submit')
            wait(client, 'hub-account', r'busy=0 signed-in=1')

    def room(client):
        state = client.inspect('room-state')
        match = re.search(r'\[room-state\] state=(\d+) local=(\S+) owner=(\S+) revision=(\d+) epoch=(\d+) match=(\d+) rotation=(\d+) members=(\d+)', state)
        if not match: raise RuntimeError(f'No room state: {state}')
        return match.groups(), re.findall(r'\[room-member\] index=(\d+) peer=(\S+) seats=(\d+) active=(\d+) waiting=(\d+) ranks=([\d,]+) confirmed=(\d+)', state)

    try:
        for profile, name in [(args.profile_a, 'Twilight'), (args.profile_b, 'Guest')]:
            client = Client(args.exe.resolve(), args.iso.resolve(), profile.resolve(), output / name.lower(), name,
                            window_size=(1280, 960), chat_endpoint=args.endpoint)
            clients.append(client)
            sign_in(client)
        a, b = clients
        a.click('account-logout')
        a.click('account-confirm')
        wait(a, 'hub-account', r'busy=0 signed-in=0')
        a.click('nav-1')
        a.click('create-lobby')
        a.enter('lobby-title', 'Guest host account review')
        a.command('ui-key', 'enter')
        a.wait_cards(1)
        first, _ = room(a)
        assert first[0] == '2' and first[1] == first[2], first
        b.click('nav-1')
        b.click('refresh-lobbies')
        listing = wait(b, 'hub-lobbies', r'title=Guest host account review')
        code = re.search(r'code=([A-Z0-9]{6,12})', listing)[1]
        b.enter('lobby-code', code)
        b.click('join-lobby')
        a.wait_cards(2); b.wait_cards(2)
        before, seats = room(a)
        people = a.inspect('hub-people')
        assert re.search(rf'\[hub-person\] room=\d+ peer={re.escape(before[1])} account=\s*$', people, re.MULTILINE), people
        screenshots.append(str(a.screenshot()))
        checks.append('An unsigned guest hosts a two-client lobby')

        sign_in(a)
        wait(a, 'hub-people', rf'\[hub-person\] room=\d+ peer={re.escape(before[1])} account={re.escape(args.a_handle)}\b')
        after, after_seats = room(a)
        assert after[0:3] == before[0:3] and after[4:] == before[4:], (before, after)
        assert seats == after_seats, (seats, after_seats)
        wait(b, 'hub-people', rf'\[hub-person\] room=\d+ peer={re.escape(before[1])} account={re.escape(args.a_handle)}\b')
        screenshots.append(str(a.screenshot()))
        checks.append('Signing in attaches the verified account without reconnecting, changing host, or reassigning seats')

        a.click('nav-3')
        wait(a, 'hub-social', r'signed-in=1 busy=0 loaded=1')
        a.enter('social-account-name', args.b_handle)
        a.command('ui-key', 'enter')
        wait(a, 'hub-social', rf'busy=0.*profile-handle={re.escape(args.b_handle)}\b')
        assert 'enabled=1' in a.inspect('profile-invite')
        a.click('profile-invite')
        b.click('nav-4')
        b.click('social-refresh')
        wait(b, 'hub-social', r'busy=0.*folder=0.*count=[1-8]')
        b.click('mail-entry-0')
        wait(b, 'hub-social', r'busy=0.*opened=1.*kind=room_invite')
        screenshots.append(str(b.screenshot()))
        checks.append('The newly attached host can send a verified lobby invitation without leaving')
        a.click('nav-6'); a.click('settings-category-3'); a.click('settings-account')
        a.click('account-logout'); a.click('account-confirm')
        wait(a, 'hub-account', r'busy=0 signed-in=0')
        wait(b, 'hub-people', r'room-count=1')
        checks.append('Signing out still leaves the lobby')
        (output / 'receipt.json').write_text(json.dumps({
            'checks': checks, 'screenshots': screenshots,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['Two hidden clients on one PC; physical multi-PC and controller review remain separate.'],
        }, indent=2) + '\n')
    finally:
        for client in clients: client.stop()
    print(f'{len(checks)} native guest-account UI checks passed; {len(screenshots)} screenshots in {output}')


if __name__ == '__main__':
    main()
