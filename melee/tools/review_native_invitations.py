"""Review verified player profiles and lobby invitations in hidden, muted clients."""
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
            state = client.inspect(tag)
            if re.search(pattern, state):
                return state
            time.sleep(.2)
        raise RuntimeError(f'{tag} did not reach {pattern}: {state}')

    def click_account(client, handle):
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            regions = client.inspect('hub-regions')
            for line in regions.splitlines():
                if re.search(rf' account={re.escape(handle)}$', line):
                    match = re.search(r'origin=([\d.-]+),([\d.-]+) size=([\d.-]+),([\d.-]+) enabled=1', line)
                    if match:
                        x, y, width, height = map(float, match.groups())
                        client.command('ui-click', round(x + width / 2), round(y + height / 2))
                        return
            time.sleep(.2)
        raise RuntimeError(f'No clickable verified player {handle}: {regions}')

    def create(client, title):
        client.click('nav-1')
        client.click('create-lobby')
        client.enter('lobby-title', title)
        client.click('confirm-create-lobby')
        client.wait_cards(1)

    try:
        for profile, name in [(args.profile_a, 'Twilight'), (args.profile_b, 'Guest')]:
            client = Client(args.exe.resolve(), args.iso.resolve(), profile.resolve(), output / name.lower(), name,
                window_size=(1280, 960), chat_endpoint=args.endpoint)
            clients.append(client)
            client.click('nav-3')
            wait(client, 'hub-social', r'signed-in=1 busy=0 loaded=1.*friends=1')
        a, b = clients
        a.click('nav-2')
        people = wait(a, 'hub-people', r'chat-state=1 chat-count=2')
        assert f'account={args.a_handle}' in people and f'account={args.b_handle}' in people, people
        click_account(a, args.b_handle)
        wait(a, 'hub-social', rf'busy=0.*profile-handle={re.escape(args.b_handle)}\b')
        screenshots.append(str(a.screenshot()))
        assert 'enabled=0' in a.inspect('profile-invite')
        checks.append('The global player list opens the verified remote account profile; lobby invite is unavailable outside a lobby')

        create(a, "Twilight's lab")
        create(b, 'Other practice lobby')
        for client, handle in [(a, args.a_handle), (b, args.b_handle)]:
            people = wait(client, 'hub-people', rf'\[hub-person\] room=0 .*account={re.escape(handle)}\b')
            assert 'room-count=1' in people
        a.click('nav-3')
        a.enter('social-account-name', args.b_handle)
        a.click('find-account')
        wait(a, 'hub-social', rf'busy=0.*profile-handle={re.escape(args.b_handle)}\b')
        assert 'enabled=1' in a.inspect('profile-invite')
        a.click('profile-invite')
        wait(a, 'hub-social', r'busy=0.*invite-pending=0')
        a.click('nav-4')
        a.click('mail-sent')
        wait(a, 'hub-social', r'busy=0.*folder=1.*count=1 total=1')
        screenshots.append(str(a.screenshot()))
        checks.append('A signed-in lobby member sends a friend invitation through the native account worker into Sent')

        b.click('nav-4')
        b.click('social-refresh')
        wait(b, 'hub-social', r'busy=0.*folder=0.*count=1 total=1')
        b.click('mail-entry-0')
        wait(b, 'hub-social', r'busy=0.*opened=1.*kind=room_invite')
        screenshots.append(str(b.screenshot()))
        b.click('letter-reply')
        b.wait_cards(2)
        a.wait_cards(2)
        people = wait(b, 'hub-people', r'room-count=2')
        assert f'account={args.a_handle}' in people and f'account={args.b_handle}' in people, people
        click_account(b, args.a_handle)
        wait(b, 'hub-social', rf'busy=0.*profile-handle={re.escape(args.a_handle)}\b')
        screenshots.append(str(b.screenshot()))
        checks.append('Opening an inbox invitation checks it with the service, leaves the old lobby and joins the sender; lobby clicks open verified profiles')

        b.click('nav-6')
        b.click('settings-account')
        b.click('account-logout')
        b.click('account-confirm')
        wait(b, 'hub-account', r'busy=0 signed-in=0')
        a.click('nav-1')
        wait(a, 'hub-people', r'room-count=1')
        b.click('nav-2')
        people = wait(b, 'hub-people', r'chat-state=1 chat-count=2')
        assert f'account={args.b_handle}' not in people, people
        screenshots.append(str(b.screenshot()))
        checks.append('Signing out leaves the lobby and reconnects global chat as an unverified guest')

        (output / 'receipt.json').write_text(json.dumps({'checks': checks, 'screenshots': screenshots,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['Uses software-authenticator sessions; Windows Hello and production deployment remain unverified.']}, indent=2) + '\n')
    finally:
        for client in clients:
            client.stop()
    print(f'{len(checks)} native invitation UI checks passed; {len(screenshots)} screenshots in {output}')


if __name__ == '__main__':
    main()
