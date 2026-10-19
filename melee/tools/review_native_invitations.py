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
    parser.add_argument('--unlisted', action='store_true', help='Keep review lobbies out of the public directory')
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
        if args.unlisted:
            client.click('list-lobby')
        client.click('confirm-create-lobby')
        client.wait_cards(1)

    def wait_chat_people(client, present, absent=None):
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            state = client.inspect('hub-people')
            chat = '\n'.join(line for line in state.splitlines() if '[hub-person] chat=' in line)
            if 'chat-state=1' in state and all(f'account={handle}' in chat for handle in present) and (not absent or f'account={absent}' not in chat):
                return state
            time.sleep(.2)
        raise RuntimeError('Expected review accounts did not reach the global player list')

    try:
        for profile, name in [(args.profile_a, 'Twilight'), (args.profile_b, 'Guest')]:
            client = Client(args.exe.resolve(), args.iso.resolve(), profile.resolve(), output / name.lower(), name,
                window_size=(1280, 960), chat_endpoint=args.endpoint)
            clients.append(client)
            client.click('nav-3')
            wait(client, 'hub-social', r'signed-in=1 busy=0 loaded=1.*friends=1')
        a, b = clients
        wait(a, 'hub-social', rf'\[hub-friend\] handle={re.escape(args.b_handle)} relationship=friend presence=Online')
        screenshots.append(str(a.screenshot()))
        checks.append('Friends shows a verified account online while both clients are connected')
        a.click('nav-2')
        people = wait_chat_people(a, [args.a_handle, args.b_handle])
        assert f'account={args.a_handle}' in people and f'account={args.b_handle}' in people, people
        click_account(a, args.b_handle)
        wait(a, 'hub-social', rf'busy=0.*profile-handle={re.escape(args.b_handle)}\b')
        screenshots.append(str(a.screenshot()))
        assert 'enabled=1' in a.inspect('profile-invite')
        checks.append('The global player list opens a verified friend profile with Create & invite available outside a lobby')

        a.click('profile-invite')
        a.element('confirm-create-lobby')
        assert f'create-invite={args.b_handle} create-queued=0' in a.inspect('hub-social')
        screenshots.append(str(a.screenshot()))
        a.command('ui-key', 'escape')
        a.element('profile-invite')
        assert 'create-invite= create-queued=0' in a.inspect('hub-social')
        b.click('nav-4')
        b.click('social-refresh')
        wait(b, 'hub-social', r'busy=0.*folder=0.*count=0 total=0')
        checks.append('Backing out of Create & invite returns to the friend profile and sends no invitation')

        a.click('profile-invite')
        a.enter('lobby-title', "Twilight's lab")
        if args.unlisted:
            a.click('list-lobby')
        a.click('confirm-create-lobby')
        a.wait_cards(1)
        create(b, 'Other practice lobby')
        for client, handle in [(a, args.a_handle), (b, args.b_handle)]:
            people = wait(client, 'hub-people', rf'\[hub-person\] room=0 .*account={re.escape(handle)}\b')
            assert 'room-count=1' in people
        a.click('nav-4')
        a.click('mail-sent')
        wait(a, 'hub-social', r'busy=0.*folder=1.*count=1 total=1')
        screenshots.append(str(a.screenshot()))
        checks.append('Creating the named lobby automatically sends exactly one invitation after verified membership and listing confirmation')

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
        b.element('account-export')
        screenshots.append(str(b.screenshot()))
        b.click('account-keys')
        b.element('account-overview')
        screenshots.append(str(b.screenshot()))
        b.click('account-overview')
        b.click('account-logout')
        b.click('account-confirm')
        wait(b, 'hub-account', r'busy=0 signed-in=0')
        a.click('nav-1')
        wait(a, 'hub-people', r'room-count=1')
        b.click('nav-2')
        people = wait_chat_people(b, [args.a_handle], absent=args.b_handle)
        assert f'account={args.b_handle}' not in people, people
        screenshots.append(str(b.screenshot()))
        checks.append('Signing out leaves the lobby and reconnects global chat as an unverified guest')
        a.click('nav-3')
        wait(a, 'hub-social', rf'\[hub-friend\] handle={re.escape(args.b_handle)} relationship=friend presence=Offline')
        screenshots.append(str(a.screenshot()))
        checks.append('The friend becomes offline after sign-out even though their guest client remains connected')

        (output / 'receipt.json').write_text(json.dumps({'checks': checks, 'screenshots': screenshots,
            'exe_sha256': hashlib.sha256(args.exe.read_bytes()).hexdigest(),
            'limitations': ['Uses imported account keys and real native signatures; two clients share one PC.']}, indent=2) + '\n')
    finally:
        for client in clients:
            client.stop()
    print(f'{len(checks)} native invitation UI checks passed; {len(screenshots)} screenshots in {output}')


if __name__ == '__main__':
    main()
